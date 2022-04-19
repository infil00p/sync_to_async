#include "sync_to_async.hpp"

#include <atomic>
#include <new>
#include <utility>

void Utils::SyncToAsync::invoke(
        std::function<void(Callback)> newWork) {
    // The worker might not be waiting for work if some other invoker has
    // already sent work. Wait for the worker to be done with that work and
    // ready for new work.
    std::unique_lock<std::mutex> lock(mMutex);
    waitForWork(lock);
    // Now the worker is definitely waiting for our work. Send it over.
    auto workID = assignWork(std::move(newWork));
    waitForCompletion(lock, workID);
}

void Utils::SyncToAsync::waitForCompletion(std::unique_lock<std::mutex> &lock, uint32_t workID) {
    // Wake the worker and wait for it to finish the work. There might be other
    // invokers waiting to send work as well, so `notify_all` to ensure the
    // worker wakes up. Wait for `workCount` to increase rather than for the
    // state to return to `Waiting` to make sure we wake up even if some other
    // invoker wins the race and submits more work before we acquire the lock.
    mSynchronizationCondition.notify_all();
    mSynchronizationCondition.wait(
            lock, [&]() { return mWorkCount != workID; });
}

uint32_t Utils::SyncToAsync::assignWork(std::function<void(Callback)> newWork) {
    assert(mState == ThreadState::Waiting);
    uint32_t workID = mWorkCount;
    mWork = std::move(newWork);
    mState = ThreadState::WorkAvailable;
    return workID;
}

void Utils::SyncToAsync::waitForWork(std::unique_lock<std::mutex> &lock) {
    mSynchronizationCondition.wait(
            lock, [&]() { return mState == ThreadState::Waiting; });
}

void Utils::SyncToAsync::threadIter(void *arg) {
    auto *parent = static_cast<SyncToAsync *>(arg);
    std::function<void(Callback)> currentWork = getAssignedFunc(parent);

    // Allocate a resume function that will wake the invoker and schedule us to
    // wait for more work.
    std::function<void()> synchronizerFunc = [parent, arg]() {
        // We are called, so the work was finished. Notify the invoker so it
        // will wake up and continue once we resume waiting. There might be
        // other invokers waiting to give us work, so `notify_all` to make sure
        // our invoker wakes up. Don't worry about overflow because it's a
        // reasonable assumption that no invoker will continue losing wake up
        // races for a full cycle.
        parent->mWorkCount++;
        parent->mSynchronizationCondition.notify_all();

        // Look for more work. Doing this asynchronously ensures that we
        // continue after the current call stack unwinds (avoiding constantly
        // adding to the stack, and also running any remaining code the caller
        // had, like destructors). TODO: add an option to do a synchronous call
        // here in some cases, which would avoid the time delay caused by a
        // browser setTimeout.

        // TODO: The following line loses handle of current thread for long running js functions
        //        emscripten_async_call(threadIter, arg, /* timeout */ 0);
    };
    parent->mResumeFunc = synchronizerFunc;

    // Run the work function the user gave us. Give it a pointer to the resume
    // function, which it will be responsible for calling when it's done.
    currentWork(&parent->mResumeFunc);
}

std::function<void(Utils::SyncToAsync::Callback)> Utils::SyncToAsync::getAssignedFunc(Utils::SyncToAsync *parent) {
    std::function<void(Callback)> currentWork;
    {
        // Wait until we get something to do.
        std::unique_lock<std::mutex> lock(parent->mMutex);
        parent->mSynchronizationCondition.wait(lock,
                                               [&]() {
                                                   return parent->mState == ThreadState::WorkAvailable ||
                                                          parent->mState == ThreadState::ShouldExit;
                                               });

        if (parent->mState == ThreadState::ShouldExit) {
            pthread_exit(nullptr);
        }

        assert(parent->mState == ThreadState::WorkAvailable);
        currentWork = parent->mWork;

        // Now that we have a local copy of the work, it is ok for new invokers
        // to queue up more work for us to do, so go back to `Waiting` and
        // release the lock.
        parent->mState = ThreadState::Waiting;
    }
    return currentWork;
}


void EMSCRIPTEN_KEEPALIVE resume_execution(
        const Utils::SyncToAsync::Callback callback,
        int *const statusPointer,
        int status) {
    *statusPointer = status;
    (*callback)();
}
