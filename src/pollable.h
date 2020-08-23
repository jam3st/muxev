#pragma once

#include <memory>
#include <mutex>

class Switch;

class Pollable : public std::enable_shared_from_this<Pollable> {
friend class Switch;
private:
    virtual void read(Switch& sw) = 0;
    virtual void write(Switch& sw) = 0;
    virtual void error(Switch& sw) = 0;

    int const getFd() const {
        return fd;
    }
protected:
    virtual void tryConnect(Switch &sw) {};
protected:
    int fd = -1;
    std::mutex lock;
};
