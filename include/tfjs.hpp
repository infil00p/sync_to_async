#pragma once
#include "sync_to_async.hpp"

extern "C" {
extern void import_tfjs(Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void check_pretfjs(Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void check_if_on_thread(Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void load_graph_model_from_path(const char* model_id, const char *path, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void load_layer_model_from_path(const char* model_id, const char *path, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void load_graph_model_from_buffer(const char* model_id, const char *topology, const unsigned char *weights_ptr, const int weight_size, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void load_layer_model_from_buffer(const char* model_id, const char *topology, const unsigned char *weights_ptr, const int weight_size, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void predict_in_js(const char* model_id, const float * const input_ptr, const int input_size, const int * const input_shape_ptr, float *const output_ptr, const int output_size, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
extern void dispose_model(const char* model_id, Utils::SyncToAsync::Callback fn_to_continue_in_cpp, int *status_pointer);
}

namespace tfjs {
    Utils::JsResultStatus import();
    Utils::JsResultStatus load_file(const std::string& path, const std::string& type);
    Utils::JsResultStatus load_buffer(const std::string& topology, const std::vector<unsigned char> &weights, const std::string& type);
    Utils::JsResultStatus predict(const std::vector<float> &input, const std::vector<int> &input_shape, std::vector<float> &output);
    Utils::JsResultStatus dispose(const std::string &model_name);
}// namespace tfjs
