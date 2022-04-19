#include "tfjs.hpp"
#include <iostream>

Utils::JsResultStatus tfjs::import() {
    auto isPreJs = Utils::js_executor(check_pretfjs);
    if (isPreJs == Utils::JsResultStatus::ERROR) {
        auto isThread = Utils::js_executor(check_if_on_thread);
        if (isThread == Utils::JsResultStatus::OK) {
            return Utils::js_executor(import_tfjs);
        }
        return isThread;
    }
    return isPreJs;
}

Utils::JsResultStatus tfjs::predict(const std::vector<float> &input, const std::vector<int> &input_shape, std::vector<float> &output) {
    import();
    return Utils::js_executor(predict_in_js, "123", input.data(), input.size(), input_shape.data(), output.data(), output.size());
}

Utils::JsResultStatus tfjs::dispose(const std::string &model_name) {
    import();
    return Utils::js_executor(dispose_model, "123");
}
Utils::JsResultStatus tfjs::load_file(const std::string& path, const std::string& type) {
    import();
    if (type == "graph")
    {
        return Utils::js_executor(load_graph_model_from_path, "123", path.c_str());
    }
    else if(type == "layer")
    {
        return Utils::js_executor(load_layer_model_from_path, "123", path.c_str());
    }
    return Utils::ERROR;
}
Utils::JsResultStatus tfjs::load_buffer(const std::string& topology, const std::vector<unsigned char>& weights, const std::string& type) {
    import();
    if (type == "graph")
    {
        return Utils::js_executor(load_graph_model_from_buffer, "123", topology.c_str(), weights.data(), weights.size());
    }
    else if(type == "layer")
    {
        return Utils::js_executor(load_layer_model_from_buffer, "123", topology.c_str(), weights.data(), weights.size());
    }
    return Utils::ERROR;
}
