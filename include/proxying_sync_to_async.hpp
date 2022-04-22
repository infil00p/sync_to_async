#pragma once

#include "common.hpp"
#include "sync_to_async.hpp"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <emscripten/proxying.h>
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
        int mStatus;

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

        template<typename Func, typename... Args>
        JsResultStatus invoke(Func &&func, Args... args);

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
        auto executed = mQueue.proxySync(mReturner.native_handle(), [&]() {
            func(std::forward<Args>(args)..., &mResume, &mStatus);
        });
        if (executed) {
            std::unique_lock<std::mutex> lock(mMutex);
            mCond.wait(lock, [&]() { return funcProducedResult(); });
        }
    }
    return static_cast<JsResultStatus>(mStatus);
}
