#ifndef CORE_GRAPH_GRAPH_H
#define CORE_GRAPH_GRAPH_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/graph/constants.h"
#include "core/graph/op.h"
#include "core/common/status.h"
#include "core/graph/utils.h"
#include "core/protobuf/onnx-ml.pb.h"

using namespace onnx;

namespace lotusrt
{
    class LotusRT;
};

namespace Lotus
{
    namespace Test
    {
        class TestUtils;
    }
}

namespace LotusIR
{
    typedef size_t NODEINDEX;
    typedef int64_t VERSION;
    typedef ValueInfoProto NodeArgInfo;
    typedef std::unordered_map<std::string, TensorProto> InitializedTensorSet;
    typedef std::unordered_map<std::string, TypeProto> ArgNameToTypeMap;

    class Graph;
    class GraphBase;
    class Node;
    class OpSignature;

    // Node argument definition, for both input and output,
    // including arg name, arg type (contains both type and shape).
    //
    // Design Question: in my (Ke's) opinion, shape should not be part of type.
    // We may align the protobuf design with our operator registry interface,
    // which has type specified for each operator, but no shape. Well, shape
    // should be inferred with a separate shape inference function given
    // input shapes, or input tensor data sometimes.
    // With shape as part of type (current protobuf design), 
    // 1) we'll have to split the "TypeProto" into type and shape in this internal
    // representation interface so that it could be easily used when doing type
    // inference and matching with operator registry.
    // 2) SetType should be always called before SetShape, otherwise, SetShape()
    // will fail. Because shape is located in a TypeProto.
    // Thoughts?
    //
    class NodeArg
    {
    public:

        // Constructor by specifying node arg name and type&shape which is
        // optional. This is called when loading a <Graph> from <GraphProto>
        // normally.
        NodeArg(const std::string& p_name,
            const TypeProto* p_argType);

        // Get node arg name.
        const std::string& Name() const;

        // Get node arg type.
        const PTYPE& Type() const;

        void SetType(PTYPE p_type);
        void SetType(const TypeProto& p_typeProto);

        // Get node arg shape.
        // Return null pointer if there's no shape specified.
        const TensorShapeProto* Shape() const;

        // Set node arg shape.
        // Shape could only be set after setting type since shape information
        // now is part of TypeProto.
        void SetShape(const TensorShapeProto& p_shape);

        // Get node arg info proto.
        const NodeArgInfo& ToProto() const;

        // Indicates whether <*this> node arg exists or not.
        // Optional inputs are allowed in ONNX. Empty arg name represents 
        // a non-existing input argument.
        bool Exist() const;

    private:

        friend class Node;
        friend class GraphBase;
        friend class Graph;
        friend class lotusrt::LotusRT;        

        // Node arg PType.
        PTYPE m_type;

        // Node arg name, type and shape.
        NodeArgInfo m_nodeArgInfo;

        // Flag indicates whether <*this> node arg exists or not.
        bool m_exist;
    };

    // A node representation class.
    class Node {

    public:

        // An edge end. It could be input or output edge end of a node.
        // For node's input edge end, it's the source end, as the destination
        // end is the node itself.
        // For node's ouput edge end, it's the destination end, as the source
        // end is the node itself.
        class EdgeEnd
        {
        public:

            // Constructor.
            // An EdgeEnd contains a Node pointer, a NodeArg pointer.
            // NOTE: it does not own the Node pointer and NodeArg pointer.
            EdgeEnd(const Node* p_node, const NodeArg* p_nodeArg);

            // Get the <Node*> that this edge end refers to. 
            const Node* GetNode() const;

            // Get the <NodeArg*> that this edge end refers to.
            const NodeArg* GetNodeArg() const;

        private:

            const Node* m_node;

            const NodeArg* m_nodeArg;
        };

        // An iterator helper class for iterating a Node's neighbour nodes.
        class NodeConstIterator
        {
        public:

            NodeConstIterator(std::set<const Node*>::const_iterator p_iter);

            bool operator==(const NodeConstIterator& p_other) const;

            bool operator!=(const NodeConstIterator& p_other) const;

            void operator++();

            const Node* operator*();

        private:

            std::set<const Node*>::const_iterator m_iter;
        };

        // Get node index.
        NODEINDEX Index() const;

        // Get node name.
        const std::string& Name() const;

        // Get node operator type.
        const std::string& OpType() const;

        // Get the domain of the OperatorSet that specifies the operator named by <m_opType>.
        const std::string& Domain() const;

        // Get the OperatorSchema this node refers to.
        const OperatorSchema* Op() const;

        // Get node description.
        const std::string& Description() const;

        // Read/Write <*this> node's input args' definition, including name,
        // type and shape.
        const std::vector<NodeArg*>& InputDefs() const;
        std::vector<NodeArg*>& Mutable_InputDefs();

        const std::vector<int>& InputArgCount() const;
        std::vector<int>& Mutable_InputArgCount();

        // Read/Write <*this> node's output args' definition, including name,
        // type and shape.
        const std::vector<NodeArg*>& OutputDefs() const;
        std::vector<NodeArg*>& Mutable_OutputDefs();

        // Functions defined to traverse a Graph as below.
        // Read all input nodes of <*this>.
        Node::NodeConstIterator InputNodes_begin() const;
        Node::NodeConstIterator InputNodes_end() const;
        // Read all output nodes of <*this>.
        Node::NodeConstIterator OutputNodes_begin() const;
        Node::NodeConstIterator OutputNodes_end() const;

        // Get input/output edges.
        const std::set<EdgeEnd*>& InputEdges() const;
        const std::set<EdgeEnd*>& OutputEdges() const;

        // Given input arg, get the source end of an input edge.
        bool InputEdgeSrcEnd(NodeArg* p_inputArg,
            /*out*/const EdgeEnd** p_inputEdgeSrcEnd) const;

        // Add a node attribute with specified attribute name and value.
        bool AddAttribute(const std::string& p_attrName, const AttributeProto& p_value);

#define ADD_ATTR_INTERFACES(TypeName)                             \
        bool AddAttribute(const std::string& p_attrName,          \
                          const TypeName& p_value);               \
        bool AddAttribute(const std::string& p_attrName,          \
                          const std::vector<TypeName>& p_values); \

        ADD_ATTR_INTERFACES(int64_t)
            ADD_ATTR_INTERFACES(float)
            ADD_ATTR_INTERFACES(std::string)
            ADD_ATTR_INTERFACES(TensorProto)
            ADD_ATTR_INTERFACES(GraphProto)

            // Clear specified node attribute.
            bool ClearAttribute(const std::string& p_attrName);

        // Get node attributes.
        const NodeAttributes& GetAttributes() const;

        // Indicates on which we will run this node in runtime.        
        // Executor will decide which device that this node will run against
        // and set it properly.
        // TODO: may change the return value type to be an ENUM.
        const std::string& GetExecutionProvider() const;
        void SetExecutionProvider(const std::string& p_execution_provider);

        // Get the corresponding <NodeProto>.
        void ToProto(NodeProto& p_proto) const;

    private:

        friend class GraphBase;
        friend class Graph;

        // Node could ONLY be constructed and owned by a <Graph>.
        Node() {}
        Node(NODEINDEX p_index, GraphBase* p_graph)
            : m_index(p_index),
            m_graph(p_graph) {}
        Node(const Node& p_other);

        // Init node per <NodeProto>.
        // <p_nameToValueInfoMap> specifies the node's inputs'/outputs' value information,
        // including name, type and shape.
        void Init(const NodeProto& p_nodeProto,
            const ArgNameToTypeMap& p_nameToType);
        void Init(const std::string& p_name,
            const std::string& p_opType,
            const std::string& p_description,
            const std::vector<NodeArg*>& p_inputArgs,
            const std::vector<NodeArg*>& p_outputArgs,
            const std::string& p_domain);

        // Node index.
        NODEINDEX m_index;

        // Node name.
        std::string m_name;

        // Node operator type.
        std::string m_opType;

        // OperatorSet domain of <m_opType).
        std::string m_domain;

        // OperatorSchema that <*this> node refers to.
        const OperatorSchema* m_op;

        // Node doc string.
        std::string m_description;

        // Node inputs' definition.
        std::vector<NodeArg*> m_inputDefs;
        // The number of inputs for each argument of the operator or function which
        // this node refers.
        // For example, <m_inputDefs> has 10 elements (inputs), and <m_inputArgCount>
        // is {4, 6}. This means that 4 elements (inputs) of <m_inputDefs> map to the
        // first argument of the operator or function, and the other 6 map to the
        // second argument.
        std::vector<int> m_inputArgCount;

        // Node outputs' definition.
        std::vector<NodeArg*> m_outputDefs;

        // Node inputs edges.
        std::set<EdgeEnd*> m_input_edges;
        // Node output edges.
        std::set<EdgeEnd*> m_output_edges;
        // Node input nodes, besides input nodes mentioned in <m_inputs> above,
        // it also contains all control input nodes;
        std::set<const Node*> m_inputNodes;
        // Control input nodes' names.
        std::set<std::string> m_controlInputs;
        // Node's output nodes.
        std::set<const Node*> m_outputNodes;

        // Device.
        std::string m_execution_provider;

        // Map from attribute name to attribute.
        // This allows attribute adding and removing.
        NodeAttributes m_attributes;

        GraphBase* m_graph;
    };

    // TODO: Graph base class.
    // It should cover the common things between function and graph.
    // Move these common things from Graph to GraphBase.
    // 1. Graph does not have attributes, while function has.
    // 2. Graph does have initializers, while function does not.
    // 3. Graph does have value_info, while function does not.
    class GraphBase
    {
    public:
        // An iterator helper to access graph nodes without copy.
        // The iterator itself does not own any data.
        class NodeIterator
        {
        public:

            // Constructor.
            NodeIterator(NODEINDEX p_currentNodeIndex, GraphBase* p_graph)
                : m_graph(p_graph),
                m_currentNodeIndex(p_currentNodeIndex)
            {
            }

            bool operator==(const NodeIterator& p_other) const;

            bool operator!=(const NodeIterator& p_other) const;

            void operator++();

            Node* operator*();

        private:

            GraphBase* m_graph;

            // it's the Node Index in <m_nodes> of the <m_graph>.
            NODEINDEX m_currentNodeIndex;
        };

        // Resolve <*this> graph to ensure it's in a good shape with full
        // functionality.
        // 1. Run through all validation rules.
        //    a. Node name and node output's names should be unique.
        //    b. Attribute match between node and op definition.
        //    c. Input/Output match between node and op definition.
        //    d. Graph is acyclic and sort nodes in topological order.
        // 2. Check & Setup inner nodes' dependency.
        // 3. Cleanup function definition lists.
        // Returns resolving status.
        virtual Status Resolve() = 0;

        // Getter and Setter for graph name.
        virtual const std::string& Name() const = 0;
        virtual void SetName(const std::string& p_name) = 0;

        virtual const std::string& Description() const = 0;
        virtual void SetDescription(const std::string& p_desription) = 0;

        // Get graph inputs/outputs.
        virtual const std::vector<const NodeArg*>& GetInputs() const = 0;
        virtual const std::vector<const NodeArg*>& GetOutputs() const = 0;

        virtual std::unordered_map<std::string, NodeArg*>* GetNodeArgMap() = 0;

        // Get node given specific node index.
        Node* GetNode(NODEINDEX p_nodeIndex) const;

        // Get node iterator to access all effective nodes in the graph.
        GraphBase::NodeIterator Nodes_begin();
        GraphBase::NodeIterator Nodes_end();

        // Max Node Index.
        NODEINDEX MaxNodeIndex() const;

        // Number of nodes in the <Graph>.
        // This is smaller than MaxNodeIndex(), since there may be nodes
        // removed during optimization.
        int NumberOfNodes() const;

        // Add, remove node from <*this> graph.
        Node* AddNode(const std::string& p_name,
            const std::string& p_opType,
            const std::string& p_description,
            const std::vector<NodeArg*>& p_inputArgs,
            const std::vector<NodeArg*>& p_outputArgs,
            const std::string& p_domain = "");
        Node* AddNode(const Node& p_other);
        bool RemoveNode(NODEINDEX p_nodeIndex);

        // Convenience method for adding a constant op
        Node* AddConstantNode(const std::string& p_name,
            const std::string& p_description,
            const std::vector<NodeArg*>& p_outputArgs,
            const TensorProto& p_tensor);

        // Add control edge into <*this> graph.
        // The <p_dstNodeIndex> node does not consume any data output by
        // <p_srcNodeIndex>, but it's designed to be executed behind.
        bool AddControlEdge(NODEINDEX p_srcNodeIndex, NODEINDEX p_dstNodeIndex);

        bool IsSourceNode(NODEINDEX p_index) const;
        bool IsSinkNode(NODEINDEX p_index) const;

        const Node* SourceNode() const;
        const Node* SinkNode() const;

        Status GetNodesInTopologicalOrder(/*out*/ std::vector<NODEINDEX>** nodes);

    protected:

        friend class Node;

        // Add source/sink nodes to <*this> graph.
        void AddSourceSinkNodes();

        // Add node with specified <p_nodeProto>.
        Node* AddNode(const NodeProto& p_nodeProto,
            const ArgNameToTypeMap& p_nameToType);

        // Graph nodes.
        // Element in <m_nodes> may be nullptr due to graph optimization.
        std::vector<std::unique_ptr<Node>> m_nodes;

        // Number of nodes.
        // Normally this is smaller than the size of <m_nodes>, as some
        // elements in <m_nodes> may be removed when doing graph optimization,
        // or some elements may be merged, etc.
        int m_numOfNodes;

        NODEINDEX m_sourceNodeIndex;
        NODEINDEX m_sinkNodeIndex;

        // A flag indicates whether <*this> graph needs to be resolved.
        bool m_graphResolveNeeded;

        bool m_graphProtoSyncNeeded;

        int m_graphType = 0;

        // The topologic order of node index.
        std::vector<NODEINDEX> m_nodesInTopologicalOrder;

        // Graph inputs.
        std::vector<const NodeArg*> m_graphInputs;

        // Graph outputs.
        std::vector<const NodeArg*> m_graphOutputs;

        const std::unordered_map<std::string, int>* m_domainToVersion;

    private:

        Node* AllocateNode();

        void ReleaseNode(NODEINDEX p_nodeIndex);
    };

    // A graph representation class.
    class Graph : public GraphBase
    {
    public:

        // An iterator helper to access graph nodes without copy.
        // The iterator itself does not own any data.
        //class NodeIterator
        //{
        //public:

        //    // Constructor.
        //    NodeIterator(NODEINDEX p_currentNodeIndex, Graph* p_graph)
        //        : m_graph(p_graph),
        //        m_currentNodeIndex(p_currentNodeIndex)
        //    {
        //    }

        //    bool operator==(const NodeIterator& p_other) const;

        //    bool operator!=(const NodeIterator& p_other) const;

        //    void operator++();

        //    Node* operator*();

        //private:

        //    Graph* m_graph;

        //    // it's the Node Index in <m_nodes> of the <m_graph>.
        //    NODEINDEX m_currentNodeIndex;
        //};

        // Resolve <*this> graph to ensure it's in a good shape with full
        // functionality.
        // 1. Run through all validation rules.
        //    a. Node name and node output's names should be unique.
        //    b. Attribute match between node and op definition.
        //    c. Input/Output match between node and op definition.
        //    d. Graph is acyclic and sort nodes in topological order.
        // 2. Check & Setup inner nodes' dependency.
        // 3. Cleanup function definition lists.
        // Returns resolving status.
        virtual Status Resolve() override;

        // Getter and Setter for graph name.
        virtual const std::string& Name() const override;
        virtual void SetName(const std::string& p_name) override;

        virtual const std::string& Description() const override;
        virtual void SetDescription(const std::string& p_desription) override;

        virtual std::unordered_map<std::string, NodeArg*>* GetNodeArgMap() override;

        // Add/Remove/Get initial tensors for some graph inputs.
        void AddInitializedTensor(const TensorProto& p_tensor);
        void RemoveInitializedTensor(const std::string& p_tensorName);
        bool GetInitializedTensor(const std::string& p_tensorName,
            TensorProto& p_value) const;
        const InitializedTensorSet& GetAllInitializedTensors() const;

        // Get graph inputs/outputs/valueinfos.
        virtual const std::vector<const NodeArg*>& GetInputs() const override;
        virtual const std::vector<const NodeArg*>& GetOutputs() const override;
        const std::vector<const NodeArg*>& GetValueInfo() const;

        // Get node given specific node index.
        //Node* GetNode(NODEINDEX p_nodeIndex);

        // Get node iterator to access all effective nodes in the graph.
        //Graph::NodeIterator Nodes_begin();
        //Graph::NodeIterator Nodes_end();

        // Max Node Index.
        //NODEINDEX MaxNodeIndex() const;

        // Number of nodes in the <Graph>.
        // This is smaller than MaxNodeIndex(), since there may be nodes
        // removed during optimization.
        // int NumberOfNodes() const;

        // Add, remove node from <*this> graph.
        //Node* AddNode(const std::string& p_name,
        //    const std::string& p_opType,
        //    const std::string& p_description,
        //    const std::vector<NodeArg>& p_inputArgs,
        //    const std::vector<NodeArg>& p_outputArgs,
        //    const std::string& p_domain = "");
        //Node* AddNode(const std::string& p_name,
        //    const std::string& p_opType,
        //    const std::string& p_description,
        //    const std::vector<NodeArg>& p_inputArgs,
        //    const std::vector<int>& p_inputArgCount,
        //    const std::vector<NodeArg>& p_outputArgs,
        //    const std::string& p_domain = "");
        //Node* AddNode(const std::string& p_name,
        //    const std::string& p_opType,
        //    const std::string& p_description,
        //    const std::vector<NodeArg>& p_outputArgs,
        //    const std::string& p_domain = "");
        //Node* AddNode(const Node& p_other);
        //bool RemoveNode(NODEINDEX p_nodeIndex);

        //// Convenience method for adding a constant op
        //Node* AddConstantNode(const std::string& p_name,
        //    const std::string& p_description,
        //    const std::vector<NodeArg>& p_outputArgs,
        //    const TensorProto& p_tensor);

        // Add control edge into <*this> graph.
        // The <p_dstNodeIndex> node does not consume any data output by
        // <p_srcNodeIndex>, but it's designed to be executed behind.
        //bool AddControlEdge(NODEINDEX p_srcNodeIndex, NODEINDEX p_dstNodeIndex);

        // Serialize the <Graph> into <GraphProto>.
        const GraphProto& ToGraphProto();

        //bool IsSourceNode(NODEINDEX p_index) const;
        //bool IsSinkNode(NODEINDEX p_index) const;

        //const Node* SourceNode() const;
        //const Node* SinkNode() const;

        //Status GetNodesInTopologicalOrder(std::vector<NODEINDEX>** nodes);

    private:

        friend class Model;

        Graph() = delete;

        // Constructor from scratch.
        // <p_isONNX> is a special flag to indicate whether it's
        // going to construct a ONNX graph. With ONNX graph, strict
        // type checking will be skiped.
        Graph(const std::string& p_name,
            const std::unordered_map<std::string, int>& p_domainToVersion,
            bool p_isONNX = false);

        // Constructor: Given a <GraphProto> loaded from model file, construct
        // a <Graph> object.
        Graph(const GraphProto& p_graphProto,
            const std::unordered_map<std::string, int>& p_domainToVersion);

        enum Type
        {
            // A main graph.
            Main = 1,
            // A sub graph (function).
            Sub = 2,
            // A graph with strict type checking.
            Strict = 4,
        };

        friend class Node;



        Status VerifyNoDuplicateName(
            /*out*/ std::unordered_map<std::string, Node*>& p_outputArgs,
            /*out*/ std::unordered_map<std::string, NODEINDEX>& p_nodeNameToIndex);

        // Build and verify node connection (edges).
        // Verify NodeArg name/type/shape matching correctly.
        Status BuildConnections(
            const std::unordered_map<std::string, Node*>& p_outputArgs,
            const std::unordered_map<std::string, NODEINDEX>& p_nodeNameToIndex);

        // Check whether <*this> graph is acyclic.
        // Depth-first going thru the graph and check whether there's any back
        // edge.
        // <p_nodesInToplogicalOrder> returns nodes' indexes in toplogical
        // order if <Status> returned is "OK", otherwise it's undefined.
        Status CheckIsAcyclic(
            /*out*/std::vector<NODEINDEX>& p_nodesInToplogicalOrder);

        // Given nodes in topological order, infer and set type information
        // across <*this> graph if needed, and verify type/attribute
        // information match between node and op.
        Status VerifyNodeAndOpMatch(
            const std::vector<NODEINDEX>& p_nodesInToplogicalOrder,
            std::unordered_map<std::string, Node*>& p_outputArgs);

        Status InferAndVerifyTypeMatch(Node* p_node,
            const OpSignature* p_op,
            const std::unordered_map<std::string, Node*>& p_outputArgs);



        // Set graph inputs/outputs when resolving a graph..
        Status SetGraphInputsOutputs();

        // Sync graph inputs/outputs when serializing to proto.
        void SyncGraphInputsOutputs();

        // Graph nodes.
        // Element in <m_nodes> may be nullptr due to graph optimization.
        //std::vector<std::unique_ptr<Node>> m_nodes;

        // Number of nodes.
        // Normally this is smaller than the size of <m_nodes>, as some
        // elements in <m_nodes> may be removed when doing graph optimization,
        // or some elements may be merged, etc.
        //int m_numOfNodes;

        //NODEINDEX m_sourceNodeIndex;
        //NODEINDEX m_sinkNodeIndex;

        // GraphProto to store name, version, initializer.
        // When serilizing <*this> Graph to a GraphProto, the nodes and
        // functions in <Graph> will also be fed into <m_graphProto> so that
        // it's consistent with <*this> graph.
        GraphProto m_graphProto;

        // The node which refers to <*this> graph (Function).
        //Node* m_node;

        InitializedTensorSet m_nameToInitialTensor;

        // A flag indicates whether <*this> graph needs to be resolved.
        //bool m_graphResolveNeeded;

        //bool m_graphProtoSyncNeeded;

        //int m_graphType = 0;

        // The topologic order of node index.
        //std::vector<NODEINDEX> m_nodesInTopologicalOrder;

        // Graph inputs.
        //std::vector<const NodeArg*> m_graphInputs;

        // Graph outputs.
        //std::vector<const NodeArg*> m_graphOutputs;

        // Graph value_info.
        std::vector<const NodeArg*> m_valueInfo;

        // Store NodeArg in this graph
        // QUESTION: what does the key represent here?
        std::unordered_map<std::string, NodeArg*> m_nodeArgs;

        //const std::unordered_map<std::string, int>* m_domainToVersion;
    };
}

#endif  // CORE_GRAPH_GRAPH_H
