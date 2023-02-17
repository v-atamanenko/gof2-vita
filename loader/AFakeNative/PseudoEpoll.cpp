#include "PseudoEpoll.h"
#include "AFakeNative_Utils.h"
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sys/unistd.h>
#include <cstdio>

#define EPOLL_FD_MARGIN 128
#define EPOLL_FD_MAX 64

#define EVENTFD_MARGIN 256
#define EVENTFD_MAX 64

typedef struct epollElement {
    int fd;
    pseudo_epoll_event e;
} epollElement;

typedef struct _epoll_fd_internal {
    int fd = -1; // >=0 indicates that it's in use
    std::map<int, epollElement> interest;
} _epoll_fd_internal;

static _epoll_fd_internal epoll_fd_pool[EPOLL_FD_MAX];
static pthread_mutex_t * _epoll_lock = nullptr;

typedef struct _eventfd_internal {
    int fd = -1; // >=0 indicates that it's in use
    uint64_t value;
    int flags;
} _eventfd_internal;

static _eventfd_internal eventfd_pool[EVENTFD_MAX];

void _check_init_lock() {
    if (_epoll_lock == nullptr) {
        pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
        _epoll_lock = (pthread_mutex_t *) calloc(1, sizeof(pthread_mutex_t));
        memcpy(_epoll_lock, &mut, sizeof(pthread_mutex_t));

        for (int i = 0; i < EVENTFD_MAX; ++i) {
            eventfd_pool[i].fd = -1;
            eventfd_pool[i].value = 0;
            eventfd_pool[i].flags = 0;
        }

        for (int i = 0; i < EPOLL_FD_MAX; ++i) {
            epoll_fd_pool[i].fd = -1;
        }
    }
}

void _lock() {
    _check_init_lock();
    pthread_mutex_lock(_epoll_lock);
}

void _unlock() {
    if (_epoll_lock) pthread_mutex_unlock(_epoll_lock);
}

int pseudo_epoll_create(int size) {
    if (size <= 0) {
        errno = EINVAL;
        return -1;
    }

    return pseudo_epoll_create1(0);
}

int pseudo_epoll_create1(int flags) {
    // flags can be ditched since the only flag is O_CLOEXEC and we never exec()?
    if (flags != 0 && flags != PSEUDO_EPOLL_CLOEXEC) {
        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == -1) {
            epoll_fd_pool[i].fd = i + EPOLL_FD_MARGIN;
            fd = &epoll_fd_pool[i];
            break;
        }
    }

    if (!fd) {
        _unlock();
        errno = EMFILE;
        return -1;
    }

    _unlock();
    return fd->fd;
}

int pseudo_epoll_ctl(int epfd, int op, int fd, struct pseudo_epoll_event *event) {
    // EBADF: epfd or fd is not a valid file descriptor.
    if (epfd < EPOLL_FD_MARGIN || epfd > EPOLL_FD_MARGIN + EPOLL_FD_MAX || fd < 0) {
        errno = EBADF;
        return -1;
    }

    // EINVAL: epfd is not an epoll file descriptor, or fd is the same as epfd
    _epoll_fd_internal * epoll = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == epfd) {
            epoll = &epoll_fd_pool[i];
            break;
        }
    }

    if (!epoll || fd == epfd) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    // EEXIST: op was EPOLL_CTL_ADD, and the supplied file descriptor fd is
    //         already registered with this epoll instance.
    if (op == PSEUDO_EPOLL_CTL_ADD && epoll->interest.contains(fd)) {
        _unlock();
        errno = EEXIST;
        return -1;
    }

    // EINVAL: An invalid event type was specified along with EPOLLEXCLUSIVE in events.
    // ????

    // EINVAL: [extra]: `event` can not be null if `op` isn't EPOLL_CTL_DEL
    if (!event && op != PSEUDO_EPOLL_CTL_DEL) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    // ENOENT: op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not registered with this epoll instance.
    if ((op == PSEUDO_EPOLL_CTL_MOD || op == PSEUDO_EPOLL_CTL_DEL) && !epoll->interest.contains(fd)) {
        _unlock();
        errno = ENOENT;
        return -1;
    }

    // EINVAL: op was EPOLL_CTL_MOD and the EPOLLEXCLUSIVE flag has previously been applied to this epfd, fd pair.
    if (op == PSEUDO_EPOLL_CTL_MOD && epoll->interest[epfd].e.events & PSEUDO_EPOLLEXCLUSIVE) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    // EINVAL: op was EPOLL_CTL_MOD and events included EPOLLEXCLUSIVE.
    if (op == PSEUDO_EPOLL_CTL_MOD && event->events & PSEUDO_EPOLLEXCLUSIVE) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    // ELOOP: fd refers to an epoll instance and this EPOLL_CTL_ADD
    //        operation would result in a circular loop of epoll
    //        instances monitoring one another or a nesting depth of
    //        epoll instances greater than 5.
    if (fd >= EPOLL_FD_MARGIN && fd < EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
        // fd refers to an epoll instance. while not exactly per spec, but let's easen up our life a bit by
        // not supporting this case
        _unlock();
        errno = ELOOP;
        return -1;
    }

    if (op == PSEUDO_EPOLL_CTL_ADD || op == PSEUDO_EPOLL_CTL_MOD) {
        printf("PSEUDO_EPOLL_CTL_ADD/MOD for fd %i", fd);
        epollElement ele;
        ele.e = *event;
        ele.fd = fd;
        epoll->interest.emplace(fd, ele);
        _unlock();
        return 0;
    }

    epoll->interest.erase(fd);
    _unlock();
    return 0;
}

int pseudo_epoll_wait(int epfd, struct pseudo_epoll_event *events, int maxevents, int timeout) {
    // fd out of our defined bounds
    if (epfd < EPOLL_FD_MARGIN || epfd > EPOLL_FD_MARGIN + EPOLL_FD_MAX) {
        errno = EBADF;
        return -1;
    }

    if (maxevents <= 0) {
        errno = EINVAL;
        return -1;
    }

    _lock();

    _epoll_fd_internal * fd = nullptr;
    for (int i = 0; i < EPOLL_FD_MAX; ++i) {
        if (epoll_fd_pool[i].fd == epfd) {
            fd = &epoll_fd_pool[i];
            break;
        }
    }

    if (!fd) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    uint64_t time_started = AFN_timeMillis();
    int eventsReported = 0;

    for (;;) {
        for (auto & e : fd->interest) {
            _eventfd_internal * efd = nullptr;
            for (int u = 0; u < EVENTFD_MAX; ++u) {
                if (eventfd_pool[u].fd == e.first) {
                    efd = &eventfd_pool[u];
                    break;
                }
            }

            if (!efd) {
                fprintf(stderr, "Unexpected epoll_wait for fd %d: not an eventfd\n", e.first);
                continue;
            }

            bool is_readable = efd->value > 0;
            bool is_writeable = efd->value < 0xfffffffffffffffe;

            if ((e.second.e.events & PSEUDO_EPOLLIN && is_readable) || (e.second.e.events & PSEUDO_EPOLLOUT && is_writeable)) {
                if (eventsReported >= maxevents) {
                    break;
                }

                memcpy(&events[eventsReported], &e.second.e, sizeof(pseudo_epoll_event));
                events[eventsReported].events = 0;
                if (e.second.e.events & PSEUDO_EPOLLIN && is_readable) events[eventsReported].events |= PSEUDO_EPOLLIN;
                if (e.second.e.events & PSEUDO_EPOLLOUT && is_writeable) events[eventsReported].events |= PSEUDO_EPOLLOUT;
                eventsReported++;
            }
        }

        if (timeout == 0) goto done;
        if (eventsReported >= maxevents) goto done;

        if (timeout != -1) {
            if (AFN_timeMillis() - time_started > timeout) goto done;
        }

        _unlock();
        usleep(10000); // give a chance for other threads to add new FDs to pool
        _lock();
    }

done:
    _unlock();
    return eventsReported;
}

ssize_t pseudo_read(int fd, void *buf, size_t count) {
    _lock();

    _eventfd_internal * efd = nullptr;
    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (efd->value == 0) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            _unlock();
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                _unlock();
                usleep(10000);
                _lock();

                if (efd->value != 0) {
                    break;
                }
            }
        }
    }

    if (efd->flags & PSEUDO_EFD_SEMAPHORE && efd->value != 0) {
        *(uint64_t *)buf = (uint64_t) 1;
        efd->value--;
        _unlock();
        return 8;
    }

    // Non-semaphore, non-zero value
    *(uint64_t *)buf = efd->value;
    efd->value = 0;
    _unlock();
    return 8;
}

ssize_t pseudo_write(int fd, const void *buf, size_t count) {
    _lock();

    _eventfd_internal * efd = nullptr;
    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        _unlock();
        errno = EINVAL;
        return -1;
    }

    uint64_t val = *(uint64_t *) buf;
    if (0xfffffffffffffffe - efd->value < val) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            _unlock();
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                _unlock();
                usleep(10000);
                _lock();

                if (0xfffffffffffffffe - efd->value >= val) {
                    break;
                }
            }
        }
    }

    efd->value += val;
    _unlock();
    return 8;
}

int pseudo_eventfd(unsigned int initval, int flags) {
    _lock();

    _eventfd_internal * fd = nullptr;
    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == -1) {
            eventfd_pool[i].fd = i + EVENTFD_MARGIN;
            fd = &eventfd_pool[i];
            break;
        }
    }

    if (!fd) {
        _unlock();
        errno = EMFILE;
        return -1;
    }

    fd->value = initval;
    fd->flags = flags;

    _unlock();
    return fd->fd;
}
