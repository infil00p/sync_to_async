function import_tfjs(fn_to_continue_in_cpp, status_pointer) {
    console.log('importing tf.js');
    try {
        importScripts('tf.min.js');
        console.log('imported tfjs');
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0)
    } catch (e) {
        console.log(e);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1)
    }

}

function check_pretfjs(fn_to_continue_in_cpp, status_pointer) {
    console.log('Checking if tf.js is given as pre-js');
    try {
        if (tf === undefined) {
            Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
            return;
        }
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    } catch (e) {
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    }
}

function check_if_on_thread(fn_to_continue_in_cpp, status_pointer) {
    console.log('Checking if we are on a thread');
    if (Module.ENVIRONMENT_IS_PTHREAD === true) {
        console.log('We are on a thread');
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    } else {
        console.log('We are not on a thread');
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    }
}

function load_graph_model_from_path(model_id, path, fn_to_continue_in_cpp, status_pointer) {
    console.log('Loading Graph model from path');
    let model_path = UTF8ToString(path);
    let name = UTF8ToString(model_id);
    tf.loadGraphModel(model_path).then((model) => {
        Module[name] = model;
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    }).catch(err => {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    });
}

function load_layer_model_from_path(model_id, path, fn_to_continue_in_cpp, status_pointer) {
    console.log('Loading layers model from path');
    let model_path = UTF8ToString(path);
    let name = UTF8ToString(model_id);

    tf.loadLayersModel(model_path).then((model) => {
        Module[name] = model;
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    }).catch(err => {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    });
}

function load_graph_model_from_buffer(model_id, topology, weights_ptr, weight_size, fn_to_continue_in_cpp, status_pointer) {
    console.log('Loading Graph model from buffer');
    let name = UTF8ToString(model_id);
    class ModelHandler
    {
        constructor(topology, weights_ptr, weight_size) {
            const array = new Uint8Array(Module.HEAPU8.buffer, weights_ptr, weight_size);
            this.model = JSON.parse(topology);
            this.model.weightData = array.buffer;
        }
        async load()
        {
            return new Promise((resolve, reject) => {
                try {
                    resolve(this.model);
                }
                catch (err)
                {
                    console.log(err);
                    throw err;
                }
            })
        }
    }
    const model = new ModelHandler(topology, weights_ptr, weight_size);
    tf.loadGraphModel(model).then((modelGraph) => {
        Module[name] = modelGraph;
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    }).catch(err => {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    });
}

function load_layer_model_from_buffer(model_id, topology, weights_ptr, weight_size, fn_to_continue_in_cpp, status_pointer) {
    console.log('Loading Layers model from buffer');
    let name = UTF8ToString(model_id);
    class ModelHandler
    {
        constructor(topology, weights_ptr, weight_size) {
            this.array = new Uint8Array(Module.HEAPU8.buffer, weights_ptr, weight_size);
            this.model = JSON.parse(topology);
            this.model.weightData = this.array.buffer;
        }
        async load()
        {
            return new Promise((resolve, reject) => {
                try {
                    resolve(this.model);
                }
                catch (err)
                {
                    console.log(err);
                    throw err;
                }
            })
        }
    }
    const model = new ModelHandler(topology, weights_ptr, weight_size);
    tf.loadLayersModel(model).then((modelGraph) => {
        Module[name] = modelGraph;
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    }).catch(err => {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    });
}

function predict_in_js(model_id, input_ptr, input_size, input_shape_ptr, output_ptr, output_size, fn_to_continue_in_cpp, status_pointer) {
    let name = UTF8ToString(model_id);
    try {
        if (!Module[name]) {
            console.log("Inference failed");
            return;
        }
        tf.tidy(() => {
            let inputBuffer = new Float32Array(Module.HEAPF32.buffer, input_ptr, input_size);
            let inputShape = new Int32Array(Module.HEAP32.buffer, input_shape_ptr, 4);
            let image_tensor = tf.tensor(inputBuffer, inputShape);
            let y = Module[name].predict(image_tensor).dataSync();
            let outputBuffer = new Float32Array(Module.HEAPF32.buffer, output_ptr, output_size);
            for (let i = 0; i < y.length; i++) {
                outputBuffer[i] = y[i];
            }
        })
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    } catch (err) {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    }
}

function dispose_model(model_id, fn_to_continue_in_cpp, status_pointer) {
    let name = UTF8ToString(model_id);

    try {
        if (!Module[name]) {
            return;
        }
        Module[name].dispose();
        Module[name] = undefined;
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 0);
    } catch (err) {
        console.log(err);
        Module._resume_execution(fn_to_continue_in_cpp, status_pointer, 1);
    }
}

function log_data(data, callback, status_pointer) {
    console.log('I am in log data');
    try {
        console.log(UTF8ToString(data));
        Module._resume_execution(callback, status_pointer, 0);
    } catch (e) {
        Module._resume_execution(callback, status_pointer, 1);
    }
}

function log_multiple(data1, data2, callback, status_pointer)
{
    try {
        console.log(UTF8ToString(data1));
        console.log(UTF8ToString(data2));
        Module._resume_execution(callback, status_pointer, 0);
    } catch (e) {
        Module._resume_execution(callback, status_pointer, 1);
    }
}

function error_func(callback, status_pointer) {
    Module._resume_execution(callback, status_pointer, 1);
}

function long_running_func(delay_in_ms, callback, status_pointer) {
    const delay = ms => new Promise(res => setTimeout(res, ms));
    delay(delay_in_ms).then(() => {
        console.log("Waited for " + delay_in_ms + " ms");
        Module._resume_execution(callback, status_pointer, 0);
    }).catch(err => {
        console.log(err);
        Module._resume_execution(callback, status_pointer, 1)
    });
}

mergeInto(LibraryManager.library, {
    import_tfjs: import_tfjs,
    check_pretfjs: check_pretfjs,
    check_if_on_thread: check_if_on_thread,
    load_graph_model_from_path: load_graph_model_from_path,
    load_layer_model_from_path: load_layer_model_from_path,
    load_graph_model_from_buffer: load_graph_model_from_buffer,
    load_layer_model_from_buffer: load_layer_model_from_buffer,
    predict_in_js: predict_in_js,
    dispose_model: dispose_model,
    log_data: log_data,
    log_multiple: log_multiple,
    error_func: error_func,
    long_running_func: long_running_func
})
