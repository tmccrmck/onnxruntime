// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <stdint.h>
#include "core/providers/cuda/shared_inc/cuda_utils.h"
#include "common.cuh"

namespace onnxruntime {
namespace cuda {

// broadcast by computing output coordinate from offset, using fast_divmod
template <typename T, typename FuncT, bool lhs_need_compute, bool rhs_need_compute>
__global__ void _BinaryElementWise(
    size_t output_rank,
    const int64_t* lhs_padded_strides,
    const T* lhs_data,
    const int64_t* rhs_padded_strides,
    const T* rhs_data,
    const fast_divmod* fdm_output_strides,
    T* output_data,
    const FuncT& functor,
    CUDA_LONG N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  CUDA_LONG lhs_index = (lhs_need_compute ? 0 : id);
  CUDA_LONG rhs_index = (rhs_need_compute ? 0 : id);
  // compute indexes with broadcasting rules: https://github.com/onnx/onnx/blob/master/docs/Broadcasting.md
  CUDA_LONG offset = id;
  for (int dim = 0; dim < output_rank; dim++) {
    int q, r;
    fdm_output_strides[dim].divmod(offset, q, r);
    // compute index increase based on stride and broadcast
    // note that stride[i-1] == stride[i] means dim[i] is 1 (broadcasting)
    if (lhs_need_compute) {
      if (lhs_padded_strides[dim] != lhs_padded_strides[dim + 1])
        lhs_index += static_cast<int>(lhs_padded_strides[dim + 1]) * q;
    }

    if (rhs_need_compute) {
      if (rhs_padded_strides[dim] != rhs_padded_strides[dim + 1])
        rhs_index += static_cast<int>(rhs_padded_strides[dim + 1]) * q;
    }
    offset = r;
  }
  output_data[id] = functor(lhs_data[lhs_index], rhs_data[rhs_index]);
}

// for scalar broadcast or non-broadcast case
template <bool IncL, bool IncR, typename T, typename FuncT>
__global__ void _BinaryElementWiseSimple(
    const T* lhs_data,
    const T* rhs_data,
    T* output_data,
    const FuncT& func,
    CUDA_LONG N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  output_data[id] = func(lhs_data[IncL ? id : 0], rhs_data[IncR ? id : 0]);
}

// for rhs per-channel broadcast case
template <typename T, typename FuncT>
__global__ void _BinaryElementWiseRhsPerChannelBatch1(
    const T* lhs_data,
    const T* rhs_data,
    const fast_divmod fdm_H,
    T* output_data,
    FuncT func,
    CUDA_LONG N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  CUDA_LONG rhs_id = fdm_H.div(id);
  output_data[id] = func(lhs_data[id], rhs_data[rhs_id]);
}

template <typename T, typename FuncT>
__global__ void _BinaryElementWiseRhsPerChannelBatchN(
    const T* lhs_data,
    const T* rhs_data,
    const fast_divmod fdm_H,
    const fast_divmod fdm_C,
    T* output_data,
    FuncT func,
    CUDA_LONG N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);
  CUDA_LONG rhs_id = fdm_H.div(id);
  int q, r;
  fdm_C.divmod(rhs_id, q, r);
  rhs_id = r;
  output_data[id] = func(lhs_data[id], rhs_data[rhs_id]);
}

template <typename T, typename FuncT>
void BinaryElementWiseNoBroadcastImpl(
    const T* lhs_data,
    const T* rhs_data,
    T* output_data,
    const FuncT& func,
    size_t count) {
  int blocksPerGrid = (int)(ceil(static_cast<float>(count) / GridDim::maxThreadsPerBlock));
  CUDA_LONG N = static_cast<CUDA_LONG>(count);
  _BinaryElementWiseSimple<true, true, T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
      lhs_data,
      rhs_data,
      output_data,
      func,
      N);
}

template <typename T, typename FuncT>
void BinaryElementWiseImpl(
    size_t output_rank_or_simple_broadcast,
    const int64_t* lhs_padded_strides,
    const T* lhs_data,
    const int64_t* rhs_padded_strides,
    const T* rhs_data,
    const fast_divmod* fdm_output_strides,
    const fast_divmod& fdm_H,
    const fast_divmod& fdm_C,
    T* output_data,
    const FuncT& func,
    size_t count) {
  int blocksPerGrid = (int)(ceil(static_cast<float>(count) / GridDim::maxThreadsPerBlock));
  CUDA_LONG N = static_cast<CUDA_LONG>(count);
  if (output_rank_or_simple_broadcast == static_cast<size_t>(SimpleBroadcast::NoBroadcast)) {
    _BinaryElementWiseSimple<true, true, T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        lhs_data,
        rhs_data,
        output_data,
        func,
        N);
  } else if (output_rank_or_simple_broadcast == static_cast<size_t>(SimpleBroadcast::LeftScalar)) {
    _BinaryElementWiseSimple<false, true, T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        lhs_data,
        rhs_data,
        output_data,
        func,
        N);
  } else if (output_rank_or_simple_broadcast == static_cast<size_t>(SimpleBroadcast::RightScalar)) {
    _BinaryElementWiseSimple<true, false, T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        lhs_data,
        rhs_data,
        output_data,
        func,
        N);
  } else if (output_rank_or_simple_broadcast == static_cast<size_t>(SimpleBroadcast::RightPerChannelBatch1)) {
    _BinaryElementWiseRhsPerChannelBatch1<T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        lhs_data,
        rhs_data,
        fdm_H,
        output_data,
        func,
        N);
  } else if (output_rank_or_simple_broadcast == static_cast<size_t>(SimpleBroadcast::RightPerChannelBatchN)) {
    _BinaryElementWiseRhsPerChannelBatchN<T, FuncT><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
        lhs_data,
        rhs_data,
        fdm_H,
        fdm_C,
        output_data,
        func,
        N);
  } else {
    if (lhs_padded_strides && rhs_padded_strides)
      _BinaryElementWise<T, FuncT, true, true><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
          output_rank_or_simple_broadcast,
          lhs_padded_strides,
          lhs_data,
          rhs_padded_strides,
          rhs_data,
          fdm_output_strides,
          output_data,
          func,
          N);
    else if (lhs_padded_strides)
      _BinaryElementWise<T, FuncT, true, false><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
          output_rank_or_simple_broadcast,
          lhs_padded_strides,
          lhs_data,
          rhs_padded_strides,
          rhs_data,
          fdm_output_strides,
          output_data,
          func,
          N);
    else
      _BinaryElementWise<T, FuncT, false, true><<<blocksPerGrid, GridDim::maxThreadsPerBlock, 0>>>(
          output_rank_or_simple_broadcast,
          lhs_padded_strides,
          lhs_data,
          rhs_padded_strides,
          rhs_data,
          fdm_output_strides,
          output_data,
          func,
          N);
  }
}

}  // namespace cuda
}  // namespace onnxruntime
