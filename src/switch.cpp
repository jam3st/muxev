#include <sys/timerfd.h>

#include <fcntl.h>
#include <csignal>
#include <iostream>
#include <functional>

#include "switch.h"
#include "exception.h"

#include "misc.h"

static std::function<void()> sigHandler = nullptr;

extern "C" {
    static void signalHandler(int const num) {
        std::cerr << "Exiting on  signal " << num <<  std::endl;
        sigHandler();
    }
}

Switch::Switch() : running(false) {
    throwIf(sigHandler != nullptr, StringException("Too many instances"));
    throwIf(0 > ::sem_init(&sem, 0, 0), StringException("sem_init"));
    ePollFd = ::epoll_create1(EPOLL_CLOEXEC);
    throwIf(0 > ePollFd, StringException("Epoll"));
    mTimerFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    throwIf(0 > mTimerFd, StringException("Monotoc Timer"));
}

void Switch::startThreads() {
    //int const nt = std::thread::hardware_concurrency();
    int nt = 1;
    for(int i = 0; i < nt; ++i) {
        threads.push_back(new Thread(this, Switch::thRun));
    }
}

void Switch::thRun(Switch* sw, Thread* th) {
    sw->runThread(*th);
}

void Switch::runThread(Thread &th) {
    try {
        while(running) {
            ::sem_wait(&sem);
            epoll_event ev;
            std::shared_ptr<Pollable> pb;
            {
                std::lock_guard<std::mutex> sync(lock);
                ev = events.front();
                events.pop_front();
                auto const it = pollables.find(ev.data.fd);
                if(pollables.end()  == it) {
                    std::cerr << "Event for deleted device " << ev.data.fd << std::endl;
                    return;
                }
                pb = it->second;
            }
            if((ev.events & EPOLLOUT) != 0) {
                std::cerr << " pollout";
                pb->write(*this);
            }
            if((ev.events & (EPOLLIN)) != 0) {
                pb->read(*this);
            }
            if((ev.events & EPOLLRDHUP) != 0 || (ev.events & EPOLLERR) != 0) {
                std::cerr << " pollerr";
                pb->error(*this);
            }
        }
    } catch(StringException const& ex) {
        std::cerr << "EXE " << ex.why() << std::endl;
        stop();
    } catch(...) {
        std::cerr << "EXE" << std::endl;
        stop();
    }
    th.exited = true;
}

void Switch::add(std::weak_ptr<Pollable> const& io, bool const hasInput) {
    std::lock_guard<std::mutex> sync(lock);
    auto sp = io.lock();
    throwIf(!sp, StringException("Unable to lock"));
    auto ioPtr = sp.get();
    //throwIf(0 > ioPtr->getFd(), StringException("Need open descriptors"));
    if(0 > ioPtr->getFd()) return ;
    throwIf(0 > ::fcntl(ioPtr->getFd(), F_SETFL, O_NONBLOCK), StringException("Need nonblocking io"));
    throwIf(!pollables.emplace(ioPtr->getFd(), sp).second, StringException("Can't add twice"));
    epoll_event event = {  (hasInput ? EPOLLIN : 0u ) | EPOLLERR | EPOLLRDHUP | EPOLLET | EPOLLWAKEUP, {.fd = ioPtr->getFd()}};
    throwIf(0 > ::epoll_ctl(ePollFd, EPOLL_CTL_ADD, ioPtr->getFd(), &event), StringException("Epoll Add"));
}

void Switch::remove(const std::weak_ptr<Pollable> &io) {
    std::lock_guard<std::mutex> sync(lock);
    auto sp = io.lock();
    throwIf(!sp, StringException("Unable to lock"));
    auto ioPtr = sp.get();
    auto it = pollables.find(ioPtr->getFd());
    throwIf(pollables.end() == it, StringException("Can't find it"));
    pollables.erase(it);
    throwIf(0 > ::epoll_ctl(ePollFd, EPOLL_CTL_DEL, ioPtr->getFd(), nullptr), StringException("Epoll Del"));
}

void Switch::main() {
    throwIf(running, StringException("Only run once"));
    throwIf(0 == pollables.size(), StringException("Need to add something first"));
    sigHandler = std::bind(&Switch::signaled, this);
    std::signal(SIGPIPE, signalHandler);
    for(int i = SIGHUP; i < _NSIG; ++i) {
          std::signal(i, SIG_IGN);
    }
    std::signal(SIGQUIT, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGUSR1, signalHandler);

    running = true;
    startThreads();
    try {
        while(running) {
            epoll_event epEvents[MAX_EPOLL_EVENTS];
            int num = ::epoll_wait(ePollFd, &epEvents[0], sizeof(epEvents)/sizeof(epEvents[0]), -1);
            if(!running) {
                break;
            }
            if(num > 0) {
                std::lock_guard<std::mutex> sync(lock);
                for(auto i = 0u; i < num; ++i) {
                    events.push_back(epEvents[i]);
                    ::sem_post(&sem);
                }
            } else if(num == -1 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            } else {
                throw StringException("EPOLL");
            }
        }
    } catch(...) {
        stop();
    }

    for(int i = SIGHUP; i < _NSIG; ++i) {
          std::signal(i, SIG_DFL);
    }
}

void Switch::stop() {
    if(running) {
        running = false;
    }
}

void Switch::signaled() {
    running = false;
}

Switch::~Switch() {
    ::sem_close(&sem);
    sigHandler = nullptr;
}
