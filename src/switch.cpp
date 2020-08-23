#include <sys/timerfd.h>
#include <unistd.h>

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
    timerFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    throwIf(0 > timerFd, StringException("Monotoc Timer"));
    epoll_event event = {EPOLLIN | EPOLLONESHOT | EPOLLET, {.u64 = 0ull}};
    throwIf(::epoll_ctl(ePollFd, EPOLL_CTL_ADD, timerFd, &event), StringException("Timer Epoll"));;
    enableTimer();
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

void Switch::enableTimer() {
    epoll_event event = {EPOLLIN | EPOLLONESHOT | EPOLLET, {.u64 = 0ull}};
    throwIf(::epoll_ctl(ePollFd, EPOLL_CTL_MOD, timerFd, &event), StringException("Timer CTL MOD"));
    itimerspec oldTimer = {};
    itimerspec newTimer = {.it_interval = {0, 0}, .it_value = {static_cast<decltype(newTimer.it_value.tv_sec)>(1),
                                                               static_cast<decltype(newTimer.it_value.tv_nsec)>(0)}};
    throwIf(::timerfd_settime(timerFd, 0, &newTimer, &oldTimer), StringException("Recoonect timer settime"));

}
void Switch::timerConnectCheck() {
    bool allConnected = true;
    for(auto const& it: pollables) {

        if(0 > it.first) {
            allConnected = false;
            it.second->tryConnect(*this);
        }
    }
    if(!allConnected) {
        enableTimer();
    }
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
                if(0ull == ev.data.fd) {
                    timerConnectCheck();
                    continue;
                } else {
                    auto const it = pollables.find(ev.data.fd);
                    if(pollables.end()  == it) {
                        std::cerr << "Event for deleted device " << ev.data.fd << std::endl;
                        return;
                    }
                    pb = it->second;
                }
            }
            if((ev.events & EPOLLRDHUP) != 0 || (ev.events & EPOLLERR) != 0) {
                pb->error(*this);
            } else {
                if ((ev.events & EPOLLOUT) != 0) {
                    pb->write(*this);
                }
                if ((ev.events & (EPOLLIN)) != 0) {
                    pb->read(*this);
                }
            }
        }
    } catch(StringException const& ex) {
        std::cerr << "Exception " << ex.why() << std::endl;
        stop();
    } catch(...) {
        std::cerr << "EXE" << std::endl;
        stop();
    }
    th.exited = true;
}
std::weak_ptr<Pollable> Switch::getFromPtr(Pollable const* io) {
    auto res = pollables.equal_range(io->getFd());

    auto it = res.first;
    for( ; it != res.second; ++it) {
        if(io == it->second.get()) {
            break;
        }
    }
    throwIf(io != it->second.get(), StringException("Can't find it"));
    return it->second;
}

void Switch::add(std::weak_ptr<Pollable> const& io, bool const hasInput) {
    auto sp = io.lock();
    throwIf(!sp, StringException("Unable to lock"));
    auto ioPtr = sp.get();
    pollables.insert(std::pair<int, std::shared_ptr<Pollable>>(ioPtr->getFd(), sp));

    if(0 > ioPtr->getFd())  {
        return;
    }
    throwIf(0 > ::fcntl(ioPtr->getFd(), F_SETFL, O_NONBLOCK), StringException("Need nonblocking io"));
    epoll_event event = {  (hasInput ? EPOLLIN : 0u ) | EPOLLERR | EPOLLRDHUP | EPOLLET | EPOLLWAKEUP, {.fd = ioPtr->getFd()}};
    throwIf(0 > ::epoll_ctl(ePollFd, EPOLL_CTL_ADD, ioPtr->getFd(), &event), StringException("Epoll Add"));
}

void Switch::remove(const std::weak_ptr<Pollable> &io) {
    auto sp = io.lock();
    throwIf(!sp, StringException("Unable to lock"));
    auto ioPtr = sp.get();
    auto res = pollables.equal_range(ioPtr->getFd());
    auto it = res.first;
    for( ; it != res.second; ++it) {
        if(sp == it->second) {
            break;
        }
    }
    throwIf(sp != it->second, StringException("Can't find it"));
    pollables.erase(it);
    if(0 > ioPtr->getFd()) {
        return;
    }
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
    exit(1);
}

void Switch::signaled() {
    running = false;
}

Switch::~Switch() {
    std::signal(SIGQUIT, SIG_DFL);
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGUSR1, SIG_DFL);
    sigHandler = nullptr;
    ::sem_close(&sem);
    ::close(timerFd);
    ::close(ePollFd);
}
