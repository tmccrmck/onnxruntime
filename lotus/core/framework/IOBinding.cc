#include "IOBinding.h"
#include "core/graph/graph.h"  // for LotusIR::ProviderType
#include "core/common/logging/logging.h"

namespace Lotus {
IOBinding::IOBinding(IExecutionProvider* p_exec_provider,
                     const Logging::Logger* p_logger)
    : p_exec_provider_(p_exec_provider),
      p_logger_(p_logger) {
}

Common::Status IOBinding::BindInput(const std::string& name, const MLValue& ml_value) {
  if (!ml_value.IsTensor()) {
    feeds_.insert({name, ml_value});
    return Status::OK();
  }

  const Tensor& src_tensor = ml_value.Get<Tensor>();
  const AllocatorInfo& src_location = src_tensor.Location();
  AllocatorPtr alloc = p_exec_provider_->GetAllocator();
  const AllocatorInfo& dst_location = alloc->Info();
  if (src_location.name == dst_location.name) {
    // no need to trigger a copy since the tensor is already at the desired location.
    feeds_.insert({name, ml_value});
    return Status::OK();
  }

  // create tensor at the desired location.
  auto element_type = src_tensor.DataType();
  auto& shape = src_tensor.Shape();
  void* buffer = alloc->Alloc(element_type->Size() * shape.Size());
  LOTUS_ENFORCE(buffer);
  std::unique_ptr<Tensor> dst_tensor = std::make_unique<Tensor>(element_type,
                                                                shape,
                                                                buffer,
                                                                dst_location,
                                                                alloc);
  Status st = p_exec_provider_->CopyTensor(src_tensor, *dst_tensor.get());
  if (!st.IsOK()) {
    return st;
  }
  MLValue dst_mlvalue;
  dst_mlvalue.Init(dst_tensor.release(),
                   DataTypeImpl::GetType<Tensor>(),
                   DataTypeImpl::GetType<Tensor>()->GetDeleteFunc());
  feeds_.insert({name, dst_mlvalue});
  return Status::OK();
}

Common::Status IOBinding::SynchronizeInputs() {
  return p_exec_provider_->Sync();
}

Common::Status IOBinding::BindOutput(const std::string& name, const MLValue& ml_value) {
  output_names_.push_back(name);
  outputs_.push_back(ml_value);
  return Status::OK();
}

const std::vector<std::string>& IOBinding::GetOutputNames() const {
  return output_names_;
}

std::vector<MLValue>& IOBinding::GetOutputs() {
  return outputs_;
}
}  // namespace Lotus
