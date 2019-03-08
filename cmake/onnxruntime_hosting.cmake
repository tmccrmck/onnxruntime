# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(onnxruntime_hosting)

# Generate .h and .cc files from protobuf file
add_library(hosting_proto ${ONNXRUNTIME_ROOT}/hosting/protobuf/predict.proto)
target_include_directories(hosting_proto PUBLIC $<TARGET_PROPERTY:protobuf::libprotobuf,INTERFACE_INCLUDE_DIRECTORIES> "${CMAKE_CURRENT_BINARY_DIR}/.." ${CMAKE_CURRENT_BINARY_DIR}/onnx)
target_compile_definitions(hosting_proto PUBLIC $<TARGET_PROPERTY:protobuf::libprotobuf,INTERFACE_COMPILE_DEFINITIONS>)
onnxruntime_protobuf_generate(APPEND_PATH IMPORT_DIRS ${ONNXRUNTIME_ROOT}/hosting/protobuf ${ONNXRUNTIME_ROOT}/core/protobuf TARGET hosting_proto)
add_dependencies(hosting_proto onnx_proto ${onnxruntime_EXTERNAL_DEPENDENCIES})

# Setup dependencies
find_package(Boost 1.69 COMPONENTS system coroutine context thread program_options REQUIRED)
set(re2_src ${REPO_ROOT}/cmake/external/re2)

# Setup source code
file(GLOB_RECURSE onnxruntime_hosting_srcs
    "${ONNXRUNTIME_ROOT}/hosting/*.h"
    "${ONNXRUNTIME_ROOT}/hosting/*.cc"
)

# For IDE only
source_group(TREE ${REPO_ROOT} FILES ${onnxruntime_hosting_srcs})

add_executable(${PROJECT_NAME} ${onnxruntime_hosting_srcs})
add_dependencies(${PROJECT_NAME} hosting_proto onnx_proto ${onnxruntime_EXTERNAL_DEPENDENCIES})

onnxruntime_add_include_to_target(${PROJECT_NAME} onnxruntime_session gsl hosting_proto)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${ONNXRUNTIME_ROOT}
        ${CMAKE_CURRENT_BINARY_DIR}/onnx
        PUBLIC
        ${Boost_INCLUDE_DIR}
        ${re2_src}
        ${ONNXRUNTIME_ROOT}/hosting/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        hosting_proto
        ${Boost_LIBRARIES}
        ${PROVIDERS_MKLDNN}
        ${MKLML_SHARED_LIB}
        ${PROVIDERS_CUDA}
        ${onnxruntime_tvm_libs}
        onnxruntime_session
        onnxruntime_optimizer
        onnxruntime_providers
        onnxruntime_util
        onnxruntime_framework
        onnxruntime_util
        onnxruntime_graph
        onnxruntime_common
        onnxruntime_mlas
        onnx
        onnx_proto
        protobuf::libprotobuf
        re2
        ${onnxruntime_EXTERNAL_LIBRARIES}
)

