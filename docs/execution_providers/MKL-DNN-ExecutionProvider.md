# MKL-DNN Execution Provider

Intel® Math Kernel Library for Deep Neural Networks (Intel® MKL-DNN) is an open-source performance library for deep-learning applications. The library accelerates deep-learning applications and frameworks on Intel® architecture and Intel® Processor Graphics Architecture. Intel MKL-DNN contains vectorized and threaded building blocks that you can use to implement deep neural networks (DNN) with C and C++ interfaces. For more visit MKL-DNN documentation at (https://intel.github.io/mkl-dnn/)

Intel and Microsoft have developed MKL-DNN Execution Provider (EP) for ONNX Runtime to accelerate performance of ONNX Runtime using Intel® Math Kernel Library for Deep Neural Networks (Intel® MKL-DNN) optimized primitives

## MKL-DNN/MKLML
To build ONNX Runtime with MKL-DNN support, build it with `./build.sh --use_mkldnn`

To build ONNX Runtime using MKL-DNN built with dependency on MKL small libraries, build it with `./build.sh --use_mkldnn --use_mklml`

## Supported OS
* Ubuntu 16.04
* Windows 10 
* Mac OS X

## Supported backend
*	CPU
* More to be added soon!

## Using the nGraph execution provider
### C/C++
The MKLDNNExecutionProvider execution provider needs to be registered with ONNX Runtime to enable in the inference session.
```
InferenceSession session_object{so};
session_object.RegisterExecutionProvider(std::make_unique<::onnxruntime:: MKLDNNExecutionProvider >());
status = session_object.Load(model_file_name);
```
The C API details are [here](https://github.com/Microsoft/onnxruntime/blob/master/docs/C_API.md#c-api).

## Python
When using the python wheel from the ONNX Runtime built with MKL-DNN execution provider, it will be automatically prioritized over the CPU execution provider. Python APIs details are [here](https://github.com/Microsoft/onnxruntime/blob/master/docs/python/api_summary.rst#api-summary).

## Using onnxruntime_perf_test and onnx_test_runner
You can test the performance of your ONNX Model with the MKL-DNN execution provider. Use the flag -e mkldnn in [onnxruntime_perf_test](https://github.com/Microsoft/onnxruntime/tree/master/onnxruntime/test/perftest#onnxruntime-performance-test) and [onnx_test_runner](https://github.com/Microsoft/onnxruntime/tree/master/onnxruntime/test/onnx/README.txt)..
