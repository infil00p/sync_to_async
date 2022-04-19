# Call Js functions from C++

Note: This is derived from [Emscripten PR](https://github.com/emscripten-core/emscripten/pull/15611)

## Usage
function defined in `js` side
```javascript
function log_data(data, callback, status_pointer) {
     try {
         console.log(data);
         Module._resume_execution(callback, status_pointer, 0);
     }
     catch(err) {
         console.log(err);
         Module._resume_execution(callback, status_pointer, 1);
     }
}
```
`C++` interface
```c++
#include "sync_to_async.hpp"
 
 extern "C"
 {
    extern void log_data(const char* data,
                         Utils::SyncToAsync::Callback fn_to_continue_in_cpp,
                         int *status_pointer);
 }
```
Call site
```c++
#include "sync_to_async.hpp"
...
auto status = Utils::js_executor(log_data, "Hello World!");
// status can be JsResultStatus::OK, ERROR, or NOT_STARTED
...
```
