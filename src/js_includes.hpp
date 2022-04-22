extern "C" {
extern void log_data(
        const char *data, std::function<void()> *callback, int *statusPointer);

extern void log_multiple(const char *data1,
                         const char *data2,
                         std::function<void()> *callback,
                         int *statusPointer);

extern void error_func(std::function<void()> *callback, int *statusPointer);

extern void long_running_func(
        int delay_in_ms, std::function<void()> *callback, int *statusPointer);
}
