#include "ALooper.h"
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <vector>
#include <cstring>
#include <unordered_map>

#include "AFakeNative_Utils.h"
#include "PseudoEpoll.h"

// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;

constexpr uint64_t WAKE_EVENT_FD_SEQ = 1;

pthread_key_t key;
bool tls_initialized = false;

using SequenceNumber = uint64_t;

struct Request {
    int fd;
    int ident;
    int events;
    ALooper_callbackFunc callback;
    void* data;

    uint32_t getEpollEvents() const;
};

struct Response {
    SequenceNumber seq;
    int events;
    Request request;
};


/**
 * A message that can be posted to a Looper.
 */
struct Message {
    Message() : what(0) { }
    Message(int w) : what(w) { }

    /* The message type. (interpretation is left up to the handler) */
    int what;
};

/**
 * Interface for a Looper message handler.
 *
 * The Looper holds a strong reference to the message handler whenever it has
 * a message to deliver to it.  Make sure to call Looper::removeMessages
 * to remove any pending messages destined for the handler so that the handler
 * can be destroyed.
 */
class MessageHandler {
protected:
    virtual ~MessageHandler();

public:
    /**
     * Handles a message.
     */
    virtual void handleMessage(const Message& message) = 0;
};

struct MessageEnvelope {
    MessageEnvelope() : uptime(0) { }

    MessageEnvelope(uint64_t u, MessageHandler * h, const Message& m)
            : uptime(u), handler(h), message(m) {}

    uint64_t uptime;
    MessageHandler * handler;
    Message message;
};


struct internal_ALooper {
    bool mAllowNonCallbacks; // immutable

    int mWakeEventFd;  // immutable
    pthread_mutex_t mLock;

    std::vector<MessageEnvelope> mMessageEnvelopes; // guarded by mLock
    bool mSendingMessage; // guarded by mLock

    // Whether we are currently waiting for work.  Not protected by a lock,
    // any use of it is racy anyway.
    volatile bool mPolling;

    int mEpollFd;  // guarded by mLock but only modified on the looper thread
    bool mEpollRebuildRequired; // guarded by mLock

    // Locked maps of fds and sequence numbers monitoring requests.
    // Both maps must be kept in sync at all times.
    std::unordered_map<SequenceNumber, Request> mRequests;               // guarded by mLock
    std::unordered_map<int /*fd*/, SequenceNumber> mSequenceNumberByFd;  // guarded by mLock

    // The sequence number to use for the next fd that is added to the looper.
    // The sequence number 0 is reserved for the WakeEventFd.
    SequenceNumber mNextRequestSeq;  // guarded by mLock

    // This state is only used privately by pollOnce and does not require a lock since
    // it runs on a single thread.
    std::vector<Response> mResponses;
    size_t mResponseIndex;
    uint64_t mNextMessageUptime; // set to LLONG_MAX when none
};

void rebuildEpollLocked(internal_ALooper * self);

extern "C" void __destr_fn(void *parm)
{
    printf("Destructor function invoked\n");
    free(parm);
}

ALooper* ALooper_forThread() {
    if (!tls_initialized) {
        if (pthread_key_create(&key, __destr_fn ) != 0) {
            printf("pthread_key_create failed, errno=%d", errno);
        } else {
            tls_initialized = true;
        }
        return nullptr;
    }
    // Will return NULL if there is no ALooper for the thread, as per specification.
    return (ALooper*) pthread_getspecific(key);
}

ALooper* ALooper_prepare(int opts) {
    ALooper * ret = ALooper_forThread();
    if (ret != nullptr) return ret;

    auto * ial = (internal_ALooper *) malloc(sizeof(internal_ALooper));
    ial->mAllowNonCallbacks = opts == ALOOPER_PREPARE_ALLOW_NON_CALLBACKS;
    ial->mSendingMessage = false;
    ial->mPolling = false;
    ial->mEpollRebuildRequired = false;
    ial->mNextRequestSeq = WAKE_EVENT_FD_SEQ + 1;
    ial->mResponseIndex = 0;
    ial->mNextMessageUptime = LLONG_MAX;
    ial->mWakeEventFd = pseudo_eventfd(0, PSEUDO_EFD_NONBLOCK | PSEUDO_EFD_CLOEXEC);
    LOG_ALWAYS_FATAL_IF(ial->mWakeEventFd < 0, "Could not make wake event fd: %s", strerror(errno));

    pthread_mutex_init(&ial->mLock, NULL);
    rebuildEpollLocked(ial);

    if (tls_initialized) {
        if (pthread_setspecific(key, ial) != 0) {
            printf("ALooper_prepare: pthread_setspecific failed with errno %d\n", errno);
        }
    }

    return (ALooper *) ial;
}

pseudo_epoll_event createEpollEvent(uint32_t events, uint64_t seq) {
    return {.events = events, .data = {.u64 = seq}};
}

void wake(internal_ALooper * self) {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ wake", this);
#endif

    uint64_t inc = 1;
    ssize_t nWrite = TEMP_FAILURE_RETRY(pseudo_write(self->mWakeEventFd, &inc, sizeof(uint64_t)));
    if (nWrite != sizeof(uint64_t)) {
        if (errno != EAGAIN) {
            LOG_ALWAYS_FATAL("Could not write wake signal to fd %d (returned %zd): %s",
                             self->mWakeEventFd, nWrite, strerror(errno));
        }
    }
}


void awoken(internal_ALooper * self) {
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ awoken", this);
#endif

    uint64_t counter;
    TEMP_FAILURE_RETRY(pseudo_read(self->mWakeEventFd, &counter, sizeof(uint64_t)));
}

void scheduleEpollRebuildLocked(internal_ALooper * self) {
    if (!self->mEpollRebuildRequired) {
#if DEBUG_CALLBACKS
        ALOGD("%p ~ scheduleEpollRebuildLocked - scheduling epoll set rebuild", this);
#endif
        self->mEpollRebuildRequired = true;
        wake(self);
    }
}

int removeSequenceNumberLocked(internal_ALooper * self, SequenceNumber seq) {
#if DEBUG_CALLBACKS
    ALOGD("%p ~ removeFd - fd=%d, seq=%u", this, fd, seq);
#endif

    const auto& request_it = self->mRequests.find(seq);
    if (request_it == self->mRequests.end()) {
        return 0;
    }
    const int fd = request_it->second.fd;

    // Always remove the FD from the request map even if an error occurs while
    // updating the epoll set so that we avoid accidentally leaking callbacks.
    self->mRequests.erase(request_it);
    self->mSequenceNumberByFd.erase(fd);

    int epollResult = pseudo_epoll_ctl(self->mEpollFd, PSEUDO_EPOLL_CTL_DEL, fd, nullptr);
    if (epollResult < 0) {
        if (errno == EBADF || errno == ENOENT) {
            // Tolerate EBADF or ENOENT because it means that the file descriptor was closed
            // before its callback was unregistered. This error may occur naturally when a
            // callback has the side-effect of closing the file descriptor before returning and
            // unregistering itself.
            //
            // Unfortunately due to kernel limitations we need to rebuild the epoll
            // set from scratch because it may contain an old file handle that we are
            // now unable to remove since its file descriptor is no longer valid.
            // No such problem would have occurred if we were using the poll system
            // call instead, but that approach carries other disadvantages.
#if DEBUG_CALLBACKS
            ALOGD("%p ~ removeFd - EPOLL_CTL_DEL failed due to file descriptor "
                  "being closed: %s",
                  this, strerror(errno));
#endif
            scheduleEpollRebuildLocked(self);
        } else {
            // Some other error occurred.  This is really weird because it means
            // our list of callbacks got out of sync with the epoll set somehow.
            // We defensively rebuild the epoll set to avoid getting spurious
            // notifications with nowhere to go.
            printf("Error removing epoll events for fd %d: %s\n", fd, strerror(errno));
            scheduleEpollRebuildLocked(self);
            return -1;
        }
    }
    return 1;
}

void rebuildEpollLocked(internal_ALooper * self) {
    // Close old epoll instance if we have one.
    if (self->mEpollFd >= 0) {
#if DEBUG_CALLBACKS
        ALOGD("%p ~ rebuildEpollLocked - rebuilding epoll set", this);
#endif
        self->mEpollFd = -1;
    }

    // Allocate the new epoll instance and register the WakeEventFd.
    self->mEpollFd = pseudo_epoll_create1(PSEUDO_EPOLL_CLOEXEC);
    LOG_ALWAYS_FATAL_IF(self->mEpollFd < 0, "Could not create epoll instance: %s", strerror(errno));

    pseudo_epoll_event wakeEvent = createEpollEvent(PSEUDO_EPOLLIN, WAKE_EVENT_FD_SEQ);
    int result = pseudo_epoll_ctl(self->mEpollFd, PSEUDO_EPOLL_CTL_ADD, self->mWakeEventFd, &wakeEvent);
    LOG_ALWAYS_FATAL_IF(result != 0, "Could not add wake event fd to epoll instance: %s",
                        strerror(errno));

    for (const auto& [seq, request] : self->mRequests) {
        pseudo_epoll_event eventItem = createEpollEvent(request.getEpollEvents(), seq);

        int epollResult = pseudo_epoll_ctl(self->mEpollFd, PSEUDO_EPOLL_CTL_ADD, request.fd, &eventItem);
        if (epollResult < 0) {
            printf("Error adding epoll events for fd %d while rebuilding epoll set: %s\n",
                  request.fd, strerror(errno));
        }
    }
}

#define LOOPER_GET_SELF \
    ALooper * __self = ALooper_forThread(); \
    if (!__self) { \
        printf("%s: could not get looper for thread.\n", __func__); \
        return ALOOPER_POLL_ERROR; \
    }                   \
    internal_ALooper * self = (internal_ALooper *) __self;

int pollInner (int timeoutMillis) {
    LOOPER_GET_SELF

#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - waiting: timeoutMillis=%d", this, timeoutMillis);
#endif

    // Adjust the timeout based on when the next message is due.
    if (timeoutMillis != 0 && self->mNextMessageUptime != LLONG_MAX) {
        uint64_t now = AFN_timeMillis();
        uint64_t messageTimeoutMillis = self->mNextMessageUptime - now;
        if (messageTimeoutMillis >= 0
            && (timeoutMillis < 0 || messageTimeoutMillis < timeoutMillis)) {
            timeoutMillis = (int) messageTimeoutMillis;
        }
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - next message in %" PRId64 "ns, adjusted timeout: timeoutMillis=%d",
                this, mNextMessageUptime - now, timeoutMillis);
#endif
    }

    // Poll.
    int result = ALOOPER_POLL_WAKE;
    self->mResponses.clear();
    self->mResponseIndex = 0;

    // We are about to idle.
    self->mPolling = true;

    struct pseudo_epoll_event eventItems[EPOLL_MAX_EVENTS];
    int eventCount = pseudo_epoll_wait(self->mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);

    // No longer idling.
    self->mPolling = false;

    // Acquire lock.
    pthread_mutex_lock(&self->mLock);

    // Rebuild epoll set if needed.
    if (self->mEpollRebuildRequired) {
        self->mEpollRebuildRequired = false;
        rebuildEpollLocked(self);
        goto Done;
    }

    // Check for poll error.
    if (eventCount < 0) {
        if (errno == EINTR) {
            goto Done;
        }
        printf("Poll failed with an unexpected error: %s\n", strerror(errno));
        result = ALOOPER_POLL_ERROR;
        goto Done;
    }

    // Check for poll timeout.
    if (eventCount == 0) {
#if DEBUG_POLL_AND_WAKE
        ALOGD("%p ~ pollOnce - timeout", this);
#endif
        result = ALOOPER_POLL_TIMEOUT;
        goto Done;
    }

    // Handle all events.
#if DEBUG_POLL_AND_WAKE
    ALOGD("%p ~ pollOnce - handling events from %d fds", this, eventCount);
#endif

    for (int i = 0; i < eventCount; i++) {
        const SequenceNumber seq = eventItems[i].data.u64;
        uint32_t epollEvents = eventItems[i].events;
        if (seq == WAKE_EVENT_FD_SEQ) {
            if (epollEvents & PSEUDO_EPOLLIN) {
                awoken(self);
            } else {
                printf("Ignoring unexpected epoll events 0x%x on wake event fd.", epollEvents);
            }
        } else {
            const auto& request_it = self->mRequests.find(seq);
            if (request_it != self->mRequests.end()) {
                const auto& request = request_it->second;
                int events = 0;
                if (epollEvents & PSEUDO_EPOLLIN) events |= ALOOPER_EVENT_INPUT;
                if (epollEvents & PSEUDO_EPOLLOUT) events |= ALOOPER_EVENT_OUTPUT;
                if (epollEvents & PSEUDO_EPOLLERR) events |= ALOOPER_EVENT_ERROR;
                if (epollEvents & PSEUDO_EPOLLHUP) events |= ALOOPER_EVENT_HANGUP;
                Response r = {.seq = seq, .events = events, .request = request};
                self->mResponses.emplace_back(r);
            } else {
                printf("Ignoring unexpected epoll events 0x%x for sequence number %llu that is no longer registered.",
                        epollEvents, seq);
            }
        }
    }
    Done: ;

    // Invoke pending message callbacks.
    self->mNextMessageUptime = LLONG_MAX;
    while (self->mMessageEnvelopes.size() != 0) {
        uint64_t now = AFN_timeMillis();
        const MessageEnvelope& messageEnvelope = self->mMessageEnvelopes.at(0);
        if (messageEnvelope.uptime <= now) {
            // Remove the envelope from the list.
            // We keep a strong reference to the handler until the call to handleMessage
            // finishes.  Then we drop it so that the handler can be deleted *before*
            // we reacquire our lock.
            { // obtain handler
                MessageHandler * handler = messageEnvelope.handler;
                Message message = messageEnvelope.message;
                self->mMessageEnvelopes.erase(self->mMessageEnvelopes.begin());
                self->mSendingMessage = true;
                pthread_mutex_unlock(&self->mLock);

#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                ALOGD("%p ~ pollOnce - sending message: handler=%p, what=%d",
                        this, handler.get(), message.what);
#endif
                handler->handleMessage(message);
            } // release handler

            pthread_mutex_lock(&self->mLock);
            self->mSendingMessage = false;
            result = ALOOPER_POLL_CALLBACK;
        } else {
            // The last message left at the head of the queue determines the next wakeup time.
            self->mNextMessageUptime = messageEnvelope.uptime;
            break;
        }
    }

    // Release lock.
    pthread_mutex_unlock(&self->mLock);

    // Invoke all response callbacks.
    for (size_t i = 0; i < self->mResponses.size(); i++) {
        Response& response = self->mResponses.at(i);
        if (response.request.ident == ALOOPER_POLL_CALLBACK) {
            int fd = response.request.fd;
            int events = response.events;
            void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
            ALOGD("%p ~ pollOnce - invoking fd event callback %p: fd=%d, events=0x%x, data=%p",
                    this, response.request.callback.get(), fd, events, data);
#endif
            // Invoke the callback.  Note that the file descriptor may be closed by
            // the callback (and potentially even reused) before the function returns so
            // we need to be a little careful when removing the file descriptor afterwards.
            if (response.request.callback != nullptr) {
                int callbackResult = response.request.callback(fd, events, data);
                if (callbackResult == 0) {
                    pthread_mutex_lock(&self->mLock);
                    removeSequenceNumberLocked(self, response.seq);
                    pthread_mutex_unlock(&self->mLock);
                }
            }

            // Clear the callback reference in the response structure promptly because we
            // will not clear the response vector itself until the next poll.
            response.request.callback = nullptr;
            result = ALOOPER_POLL_CALLBACK;
        }
    }
    return result;
}

int ALooper_pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    LOOPER_GET_SELF

    int result = 0;
    for (;;) {
        while (self->mResponseIndex < self->mResponses.size()) {
            const Response& response = self->mResponses.at(self->mResponseIndex++);
            int ident = response.request.ident;
            if (ident >= 0) {
                int fd = response.request.fd;
                int events = response.events;
                void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE
                ALOGD("%p ~ pollOnce - returning signalled identifier %d: "
                        "fd=%d, events=0x%x, data=%p",
                        this, ident, fd, events, data);
#endif
                if (outFd != nullptr) *outFd = fd;
                if (outEvents != nullptr) *outEvents = events;
                if (outData != nullptr) *outData = data;
                return ident;
            }
        }

        if (result != 0) {
#if DEBUG_POLL_AND_WAKE
            ALOGD("%p ~ pollOnce - returning result %d", this, result);
#endif
            if (outFd != nullptr) *outFd = 0;
            if (outEvents != nullptr) *outEvents = 0;
            if (outData != nullptr) *outData = nullptr;
            return result;
        }

        result = pollInner(timeoutMillis);
    }

}

int ALooper_pollAll(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    if (timeoutMillis <= 0) {
        int result;
        do {
            result = ALooper_pollOnce(timeoutMillis, outFd, outEvents, outData);
        } while (result == ALOOPER_POLL_CALLBACK);
        return result;
    } else {
        uint64_t endTime = AFN_timeMillis() + timeoutMillis;

        for (;;) {
            int result = ALooper_pollOnce(timeoutMillis, outFd, outEvents, outData);
            if (result != ALOOPER_POLL_CALLBACK) {
                return result;
            }

            uint64_t now = AFN_timeMillis();
            if (endTime - now <= 0) {
                return ALOOPER_POLL_TIMEOUT;
            }
        }
    }

}
