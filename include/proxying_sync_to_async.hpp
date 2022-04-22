#pragma once

#include "common.hpp"
#include "sync_to_async.hpp"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <emscripten/proxying.h>
// https://github.com/emscripten-core/emscripten/blob/main/tests/pthread/test_pthread_proxying_cpp.cpp
// https://emscripten.org/docs/api_reference/proxying.h.html
#include <functional>
#include <iostream>
#include <mutex>

namespace Utils {

    class QueuedSyncToAsync {
        std::thread mReturner;
        emscripten::ProxyingQueue mQueue;
        std::mutex mMutex;
        std::condition_variable mCond;
        std::function<void()> mResume;

        // We are making this int instead of JsResultStatus to pass the reference
        // to Javascript
        int mStatus;

        // This will be the function that's running in the thread.
        // We pass a reference to ourselves as argument.
        static void *mReturnerMain(void *typeErasedMe) {
            auto typedMe = static_cast<QueuedSyncToAsync *>(typeErasedMe);
            typedMe->mQueue.execute();
            emscripten_exit_with_live_runtime();
        }

        bool funcProducedResult() const { return mStatus == OK || mStatus == ERROR; }

        QueuedSyncToAsync();
        ~QueuedSyncToAsync();

    public:
        QueuedSyncToAsync(const QueuedSyncToAsync &) = delete;
        void operator=(const QueuedSyncToAsync &) = delete;
        using Callback = std::function<void()> *;

        // We pass our function and arguments to this function
        template<typename Func, typename... Args>
        JsResultStatus invoke(Func &&func, Args... args);

        // We use a static instance to reuse thread throughout the application
        // Else deadlock is oberved if new instances are created in rapid succession
        // for example in a loop
        static QueuedSyncToAsync &getInvoker();
    };

    template<typename Func, typename... Args>
    JsResultStatus queued_js_executor(Func &&func, Args... args) {
        return QueuedSyncToAsync::getInvoker().template invoke(std::forward<Func &&>(func), std::forward<Args>(args)...);
    }

}// namespace Utils

template<typename Func, typename... Args>
Utils::JsResultStatus Utils::QueuedSyncToAsync::invoke(Func &&func, Args... args) {
    mStatus = JsResultStatus::NOT_STARTED;
    {
        // I didn't observe any difference in proxySync and proxyAsync in local testing
        // but to make sure function started executing and not just enqueued I am going
        // with proxySync
        auto executed = mQueue.proxySync(mReturner.native_handle(), [&]() {

            // We will add reference to our resume function which unlocks the thread, and
            // reference to status variable which can be set appropriately once the function
            // is executed.
            func(std::forward<Args>(args)..., &mResume, &mStatus);
        });
        if (executed) {
            // If emscripten failed to execute the function, mStatus will never change
            // To avoid deadlock lock only if function is executed. Else return NOT_STARTED
            // Caller should check status and figure out why the call failed.
            std::unique_lock<std::mutex> lock(mMutex);
            mCond.wait(lock, [&]() { return funcProducedResult(); });
        }
    }
    return static_cast<JsResultStatus>(mStatus);
}
