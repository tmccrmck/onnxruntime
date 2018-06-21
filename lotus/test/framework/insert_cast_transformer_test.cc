#include "core/framework/allocatormgr.h"
#include "core/framework/allocator.h"
#include "core/framework/insert_cast_transformer.h"
#include "core/graph/model.h"
#include "gtest/gtest.h"

namespace Lotus {
namespace Test {
typedef std::vector<LotusIR::NodeArg*> ArgMap;
TEST(TransformerTest, InsertCastGPUTest) {
  auto model = std::make_shared<LotusIR::Model>("test");
  LotusIR::Graph* graph = model->MainGraph();

  TypeProto tensor_float_16;
  tensor_float_16.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT16);
  LotusIR::NodeArg i1_def("I1", &tensor_float_16),
      i2_def("I2", &tensor_float_16),
      i3_def("I3", &tensor_float_16),
      o1_def("O1", &tensor_float_16),
      o2_def("O2", &tensor_float_16),
      o3_def("O3", &tensor_float_16);

  auto node1 = graph->AddNode("node1", "MatMul", "cpu operator1", ArgMap{&i1_def, &i2_def}, ArgMap{&o1_def});
  node1->SetExecutionProviderType(LotusIR::kCpuExecutionProvider);
  auto node2 = graph->AddNode("node2", "MatMul", "gpu operator1", ArgMap{&o1_def, &i3_def}, ArgMap{&o2_def});
  node2->SetExecutionProviderType(LotusIR::kCudaExecutionProvider);
  auto node3 = graph->AddNode("node3", "Clip", "cpu operator2", ArgMap{&o2_def}, ArgMap{&o3_def});
  node3->SetExecutionProviderType(LotusIR::kCpuExecutionProvider);

  auto status = graph->Resolve();
  ASSERT_TRUE(status.IsOK());

  InsertCastTransformer transformer("Test");
  transformer.AddKernelRegistry(&KernelRegistry::Instance());

  bool modified = true;
  EXPECT_TRUE(transformer.Apply(graph, &modified).IsOK());
  status = graph->Resolve();
  EXPECT_TRUE(status.IsOK());
  EXPECT_EQ(graph->NumberOfNodes(), 10);
  for (auto it = node1->InputNodesBegin(); it != node1->InputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
  for (auto it = node1->OutputNodesBegin(); it != node1->OutputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
  for (auto it = node2->InputNodesBegin(); it != node2->InputNodesEnd(); it++) {
    if (!graph->IsSourceNode((*it)->Index())){
      EXPECT_EQ((*it)->OpType(), "Cast");
    }
  }
  for (auto it = node2->OutputNodesBegin(); it != node2->OutputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
  for (auto it = node3->InputNodesBegin(); it != node3->InputNodesEnd(); it++) {
    if (!graph->IsSourceNode((*it)->Index())){
      EXPECT_EQ((*it)->OpType(), "Cast");
    }
  }
  for (auto it = node3->OutputNodesBegin(); it != node3->OutputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
}

TEST(TransformerTest, InsertCastAllCPUTest) {
  auto model = std::make_shared<LotusIR::Model>("test");
  LotusIR::Graph* graph = model->MainGraph();

  TypeProto tensor_float_16;
  tensor_float_16.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT16);
  LotusIR::NodeArg i1_def("I1", &tensor_float_16),
      i2_def("I2", &tensor_float_16),
      i3_def("I3", &tensor_float_16),
      o1_def("O1", &tensor_float_16),
      o2_def("O2", &tensor_float_16),
      o3_def("O3", &tensor_float_16);

  auto node1 = graph->AddNode("node1", "MatMul", "cpu operator1", ArgMap{&i1_def, &i2_def}, ArgMap{&o1_def});
  node1->SetExecutionProviderType(LotusIR::kCpuExecutionProvider);
  auto node2 = graph->AddNode("node2", "MatMul", "gpu operator1", ArgMap{&o1_def, &i3_def}, ArgMap{&o2_def});
  node2->SetExecutionProviderType(LotusIR::kCpuExecutionProvider);
  auto node3 = graph->AddNode("node3", "Clip", "cpu operator2", ArgMap{&o2_def}, ArgMap{&o3_def});
  node3->SetExecutionProviderType(LotusIR::kCpuExecutionProvider);

  auto status = graph->Resolve();
  ASSERT_TRUE(status.IsOK());

  InsertCastTransformer transformer("Test");
  transformer.AddKernelRegistry(&KernelRegistry::Instance());

  bool modified = true;
  EXPECT_TRUE(transformer.Apply(graph, &modified).IsOK());
  status = graph->Resolve();
  EXPECT_TRUE(status.IsOK());
  EXPECT_EQ(graph->NumberOfNodes(), 9);
  for (auto it = node1->InputNodesBegin(); it != node1->InputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
  for (auto it = node1->OutputNodesBegin(); it != node1->OutputNodesEnd(); it++) {
    EXPECT_NE((*it)->OpType(), "Cast");
  }
  for (auto it = node2->OutputNodesBegin(); it != node2->OutputNodesEnd(); it++) {
    EXPECT_NE((*it)->OpType(), "Cast");
  }
  for (auto it = node3->InputNodesBegin(); it != node3->InputNodesEnd(); it++) {
    if (!graph->IsSourceNode((*it)->Index())){
      EXPECT_NE((*it)->OpType(), "Cast");
    }
  }
  for (auto it = node3->OutputNodesBegin(); it != node3->OutputNodesEnd(); it++) {
    EXPECT_EQ((*it)->OpType(), "Cast");
  }
}
}  // namespace Test
}  // namespace Lotus