#include "proxying_sync_to_async.hpp"

Utils::QueuedSyncToAsync::QueuedSyncToAsync() : mReturner{mReturnerMain, this},
                                                mResume{[&] {
                                                    mCond.notify_one();
                                                }},
                                                mStatus{JsResultStatus::NOT_STARTED} {}

Utils::QueuedSyncToAsync::~QueuedSyncToAsync() {
    pthread_cancel(mReturner.native_handle());
    mReturner.join();
}

Utils::QueuedSyncToAsync &Utils::QueuedSyncToAsync::getInvoker() {
    static QueuedSyncToAsync instance;
    return instance;
}
