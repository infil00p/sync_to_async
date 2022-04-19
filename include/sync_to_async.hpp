#pragma once
/*
 * Copyright 2021 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#pragma once
#include <cassert>
#include <emscripten.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>
#include <pthread.h>

#include <functional>
#include <thread>
#include <type_traits>
#include <utility>

namespace Utils {

    enum class ThreadState : std::uint8_t {
        Waiting = 0,
        WorkAvailable,
        ShouldExit,
    };

    enum JsResultStatus : int {
        OK = 0,
        ERROR = 1,
        NOT_STARTED = 2
    };

    // Helper class for generic sync-to-async conversion. Creating an instance of
    // this class will spin up a pthread. You can then call invoke() to run code
    // on that pthread. The work done on the pthread receives a callback method
    // which lets you indicate when it finished working. The call to invoke() is
    // synchronous, while the work done on the other thread can be asynchronous,
    // which allows bridging async JS APIs to sync C++ code.
    //
    // This can be useful if you are in a location where blocking is possible (like
    // a thread, or when using PROXY_TO_PTHREAD), but you have code that is hard to
    // refactor to be async, but that requires some async operation (like waiting
    // for a JS event).
    class SyncToAsync {

        // Public API
        //==============================================================================
    public:
        // Pass around the callback as a pointer to a std::function. Using a pointer
        // means that it can be sent easily to JS, as a void* parameter to a C API,
        // etc., and also means we do not need to worry about the lifetime of the
        // std::function in user code.
        using Callback = std::function<void()> *;

        //
        // Run some work on thread. This is a synchronous (blocking) call. The
        // thread where the work actually runs can do async work for us - all it
        // needs to do is call the given callback function when it is done.
        //
        // Note that you need to call the callback even if you are not async, as the
        // code here does not know if you are async or not. For example,
        //
        //  instance.invoke([](emscripten::SyncToAsync::Callback resume) {
        //    std::cout << "Hello from sync C++ on the pthread\n";
        //    (*resume)();
        //  });
        //
        // In the async case, you would call resume() at some later time.
        //
        // It is safe to call this method from multiple threads, as it locks itself.
        // That is, you can create an instance of this and call it from multiple
        // threads freely.
        //
        void invoke(std::function<void(Callback)> newWork);

        //==============================================================================
        // End Public API

    private:
        // The dedicated worker thread.
        std::thread mExecutionThread;
        // Condition variable used for bidirectional communication between the
        // worker thread and invoking threads.
        std::condition_variable mSynchronizationCondition;
        std::mutex mMutex;

        // The current state of the worker thread. New work can only be submitted
        // when in the `Waiting` state.
        ThreadState mState;

        // Increment the count every time work is finished. This will allow invokers
        // to detect that their particular work has been completed even if some
        // other invoker wins the race and submits new work before the original
        // invoker can check for completion.
        std::atomic<uint32_t> mWorkCount{0};

        // The work that the dedicated worker thread should perform and the callback
        // that needs to be called when the work is finished.
        std::function<void(Callback)> mWork;
        std::function<void()> mResumeFunc;

        static void *threadMain(void *arg) {
            // Schedule ourselves to start processing incoming work requests.
            emscripten_async_call(threadIter, arg, 0);
            return nullptr;
        }

        // The main worker thread routine that waits for work, wakes up when work is
        // available, executes the work, then schedules itself again.
        static void threadIter(void *arg);
        // Spawn the worker thread. It starts in the `Waiting` state, so it is ready
        // to accept work requests from invokers even before it starts up.

    public:
        SyncToAsync()
            : mState{ThreadState::Waiting}, mExecutionThread(threadMain, this) {
        }

        ~SyncToAsync() {
            std::unique_lock<std::mutex> lock(mMutex);

            // We are destructing the SyncToAsync object, so we should not be racing
            // with other threads trying to perform more `invoke`s. There should
            // therefore not be any work available.
            assert(mState == ThreadState::Waiting);

            // Wake the worker and tell it to quit. Be ready to join it when it
            // does. There shouldn't be other invokers waiting to send work since we
            // are destructing the SyncToAsync, so just use `notify_one`.
            mState = ThreadState::ShouldExit;
            mSynchronizationCondition.notify_one();

            // Unlock to allow the worker to wake up and exit.
            lock.unlock();
            mExecutionThread.join();
        }
        void waitForWork(std::unique_lock<std::mutex> &lock);
        uint32_t assignWork(std::function<void(Callback)> newWork);
        void waitForCompletion(std::unique_lock<std::mutex> &lock, uint32_t workID);
        static std::function<void(Callback)> getAssignedFunc(SyncToAsync *parent);
    };

    // Following are some convenient functions created to call js functions from C++
    // First template argument is the name of the function,and it is followed by arguments to the function
    // The js functions should have 2 arguments callback, status_pointer at the end to be passed to
    // Module._resume_execution. This function will fill in those arguments
    //
    // eg:
    // function defined in js side
    // function log_data(data, callback, status_pointer) {
    //      try {
    //          console.log(data);
    //          Module._resume_execution(callback, status_pointer, 0);
    //      }
    //      catch(err) {
    //          console.log(err);
    //          Module._resume_execution(callback, status_pointer, 1);
    //      }
    // }
    //
    // header defined in C++ side
    // extern "C"
    // {
    //    extern void log_data(const char* data,
    //                         Utils::SyncToAsync::Callback fn_to_continue_in_cpp,
    //                         int *status_pointer);
    // }
    //
    //  On the call side
    //  auto status = js_executor(log_data, "Hello World!");
    //  status can be JsResultStatus::OK, ERROR, or NOT_STARTED
    template<typename Func, typename... Args>
    JsResultStatus js_executor(Func &&func, Args... args) {
        using JsInvoker = Utils::SyncToAsync;
        std::function<void()> callback;
        int status = JsResultStatus::NOT_STARTED;
        JsInvoker invoker;
        invoker.invoke(
                [&](JsInvoker ::Callback resumeFunc) {
                    callback = [&]() { func(std::forward<Args>(args)..., resumeFunc, &status); };

                    auto emscripten_call_back = [](void *callback) {
                        auto *functionRef = static_cast<JsInvoker ::Callback>(callback);
                        (*functionRef)();
                    };

                    emscripten_async_call(emscripten_call_back, &callback, 0);
                });
        return static_cast<JsResultStatus>(status);
    }

    template<typename Func>
    JsResultStatus js_executor(Func &&func) {
        using JsInvoker = Utils::SyncToAsync;
        std::function<void()> callback;
        int status = JsResultStatus::NOT_STARTED;
        JsInvoker invoker;
        invoker.invoke(
                [&](JsInvoker ::Callback resumeFunc) {
                    callback = [&]() { func(resumeFunc, std::addressof(status)); };
                    auto emscripten_call_back = [](void *callback) {
                        auto *functionRef = static_cast<JsInvoker ::Callback>(callback);
                        (*functionRef)();
                    };

                    emscripten_async_call(emscripten_call_back, std::addressof(callback), 0);
                });
        return static_cast<JsResultStatus>(status);
    }

}// namespace Utils


extern "C" {
// This is a function called from js side when it is ready to handle control back to C++
//
// status_pointer is reference to variable owned by C++ side and status is the status of function
// status = 0 means OK and status = 1 means error. This is passed appropriately from Js side
//
// callback is pointer to an internal function created by SyncToAsync which includes some synchronization primitives
// The js side should call this function at the end of execution as follows
//
// Module._resume_execution(callback, status_pointer, <0 or 1>);
void EMSCRIPTEN_KEEPALIVE resume_execution(
        Utils::SyncToAsync::Callback callback,
        int *statusPointer,
        int status);
}
