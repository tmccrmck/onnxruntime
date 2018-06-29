#pragma once
#include <stdint.h>
#include "core/providers/cuda/shared_inc/cuda_utils.h"

namespace Lotus {
namespace Cuda {

// These macros simplifies coding. To add a new op with following steps:
// 1. Add a new entry in BINARY_OPS() list
// 2. (optional) Define templated single element operator in binary_elementwise_ops_impl.cu
// 3. (optional) Implement specialized single element operator
// 4. Add op kernel class definition in binary_elementwise_ops.h
// 5. Add op kernel registration and compute specialization in binary_elementwise_ops.cc
// 6. Enable test of the newly added kernel on CUDA by changing test.Run() to test.RunOnCpuAndCuda()

#define BINARY_OPS()                   \
  BINARY_OP_NAME_EXPR(Add, (a + b))    \
  BINARY_OP_NAME_EXPR(Sub, (a - b))    \
  BINARY_OP_NAME_EXPR(Mul, (a * b))    \
  BINARY_OP_NAME_EXPR(Div, (a / b))    \
  BINARY_OP_NAME_EXPR(Pow, _Pow(a, b)) \
  BINARY_OP_NAME_EXPR(And, (a & b))    \
  BINARY_OP_NAME_EXPR(Or, (a | b))     \
  BINARY_OP_NAME_EXPR(Xor, (a ^ b))    \
  BINARY_OP_NAME_EXPR(PRelu, (a > (T)0 ? a : a * b))

// NOTE that cu files are compiled with nvcc and should not refer to any Lotus headers
// so struct BinaryElementwisePreparation cannot be used here

#define BINARY_ELEMENTWISE_IMPL_DECLARATION(name) \
  template <typename T>                           \
  void Impl_##name(                               \
      size_t output_rank_or_simple_broadcast,     \
      bool lhs_dim0_broadcast,                    \
      const int64_t* lhs_padded_strides,          \
      const T* lhs_data,                          \
      bool rhs_dim0_broadcast,                    \
      const int64_t* rhs_padded_strides,          \
      const T* rhs_data,                          \
      const fast_divmod* fdm_output_strides,      \
      T* output_data,                             \
      size_t count)

#define BINARY_OP_NAME_EXPR(name, expr) BINARY_ELEMENTWISE_IMPL_DECLARATION(name);
BINARY_OPS()
#undef BINARY_OP_NAME_EXPR

}  // namespace Cuda
}  // namespace Lotus