#pragma once

#include <sys/epoll.h>
#include <semaphore.h>

#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <deque>

#include "pollable.h"

class Switch final {
public:
    Switch();
    ~Switch();
    void main();
    void add(std::weak_ptr<Pollable> const& io, bool const hasInput = true);
    void remove(std::weak_ptr<Pollable> const& io);
    std::weak_ptr<Pollable> getFromPtr(Pollable const* io);
    void enableTimer();
    void timerConnectCheck();

private:
    void signaled();
    void stop();
    class Thread;
    static void thRun(Switch*, Thread*);
    void startThreads();
    void runThread(Thread& th);
private:
    std::atomic_bool running;
    std::vector<Thread*> threads;
    std::deque<epoll_event> events;
    std::multimap<int, std::shared_ptr<Pollable>> pollables;
    std::mutex lock;
    sem_t sem;
    int ePollFd = -1;
    int timerFd = -1;
    static constexpr int MAX_EPOLL_EVENTS = 16;
    unsigned long long uid = 0;
};

class Switch::Thread {
public:
    friend class Switch;
    Thread() = delete;
    Thread(Switch* sw, void (func(Switch*, Thread*) noexcept)) : thread(func, sw, this),
                                                                 exited(false) {
    }
private:
    std::thread thread;
    std::atomic_bool exited;
};
