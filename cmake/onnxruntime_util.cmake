# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

file(GLOB_RECURSE onnxruntime_util_srcs CONFIGURE_DEPENDS
    "${ONNXRUNTIME_ROOT}/core/util/*.h"
    "${ONNXRUNTIME_ROOT}/core/util/*.cc"
)

source_group(TREE ${ONNXRUNTIME_ROOT}/core FILES ${onnxruntime_util_srcs})

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" AND NOT MSVC)
  # For x86 platforms it is important to pass this flag to compiler. Without this gemmlowp will use slow reference code.
  # These optimizations are not enabled on MSVC so excluding it.
  message("enabling optimizations for gemmlowp")
  set_source_files_properties("${ONNXRUNTIME_ROOT}/core/util/gemmlowp_common.cc" PROPERTIES COMPILE_FLAGS "-msse4.1")
endif()

set(gemmlowp_src ${PROJECT_SOURCE_DIR}/external/gemmlowp)

add_library(onnxruntime_util ${onnxruntime_util_srcs})
target_include_directories(onnxruntime_util PRIVATE ${ONNXRUNTIME_ROOT} ${MKLML_INCLUDE_DIR} ${gemmlowp_src} PUBLIC ${eigen_INCLUDE_DIRS})
onnxruntime_add_include_to_target(onnxruntime_util onnxruntime_common onnxruntime_framework gsl onnx onnx_proto protobuf::libprotobuf)
if(UNIX)
    target_compile_options(onnxruntime_util PUBLIC "-Wno-error=comment")
endif()
set_target_properties(onnxruntime_util PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(onnxruntime_util PROPERTIES FOLDER "ONNXRuntime")
add_dependencies(onnxruntime_util ${onnxruntime_EXTERNAL_DEPENDENCIES})
if (WIN32)
    target_compile_definitions(onnxruntime_util PRIVATE _SCL_SECURE_NO_WARNINGS)
    target_compile_definitions(onnxruntime_framework PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
