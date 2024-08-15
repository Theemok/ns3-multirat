#ifndef MULTI_RAT_UTILITY_H
#define MULTI_RAT_UTILITY_H

#include <vector>
#include <map>
#include "ns3/ptr.h"
#include "multi-rat-fileprotocol-helper.h"

namespace ns3 {

class Node;
class NodeContainer;
class ApplicationContainer;
class AnimationInterface;
class MultiRatFileProtocolHelper;

/**
 * @brief Holds information used to configure multi-rat nodes.
 */
struct NodeInfo {
  uint32_t x; // node x location
  uint32_t y; // node y location
};


/**
 * @brief Holds information used to configure multi-rat nodes.
 */
struct MultiRatInfo {
  std::vector<uint32_t> channels; // channels multi-rat node can access
  uint32_t x; // multi-rat node x location
  uint32_t y; // multi-rat node y location
};


/**
 * @brief Holds information used to configure networks.
 */
struct NetworkInfo {
  std::string bitrate; // speed of network
  uint32_t range; // range of a node in the network
};

/**
 * @brief Holds static utility functions to create and test multi-rat networks.
 */
class MultiRatUtility {
public:

    /**
     * @brief Sets the position of given node.
     * @param node node to set the position of
     * @param x x-coordinate to set
     * @param y y-coordinate to set
     */
    static void SetNodePosition(Ptr<Node> node, int32_t x, int32_t y);

    /**
     * @brief Creates a multi-rat network based on given import files.
     * @param networkfile file containing network information
     * @param nodefile file containing node information
     * @param multiratfile file containing multi-rat node information
     * @return vector with node containers, starting with the multi-rat nodes and followed by nodes seperated by channels
     */
    static std::vector<NodeContainer> ImportNetwork(std::string networkfile, std::string nodefile, std::string multiratfile);

    /**
     * @brief Read multi-rat node information from file.
     * @param nodefile file to read from
     * @return vector with information about multi-rat nodes
     */
    static std::vector<MultiRatInfo> ReadMultiRatInfo(std::string nodefile);

    /**
     * @brief Read node information from file.
     * @param nodefile file to read from
     * @return map with node information seperated by channel
     */
    static std::map<uint32_t, std::vector<NodeInfo>> ReadNodeInfo(std::string nodefile);

    /**
     * @brief Read network information from file.
     * @param networkfile file to read from
     * @return map with network information seperated by channel
     */
    static std::map<uint32_t, NetworkInfo> ReadNetworkInfo(std::string networkfile);

    /**
     * @brief Configure multi-rat nodes based on information given.
     * @param mrnodeinfo vector with multi-rat node information
     * @return node container with multi-rat nodes
     */
    static NodeContainer ConfigureMultiRatNodes(std::vector<MultiRatInfo> mrnodeinfo);

    /**
     * @brief Configure nodes based on information given.
     * @param nodeinfo vector with multi-rat node information
     * @return node container with nodes
     */
    static NodeContainer ConfigureNodes(std::vector<NodeInfo> nodeinfo);

    /**
     * @brief Configure nodes based on information given.
     * @param nodes nodes that belong in the network
     * @param baseAddress address range to assign, mask is 255.255.0.0
     * @param networkinfo information about the network
     */
    static void ConfigureNetwork(NodeContainer nodes, std::string baseAddress, NetworkInfo networkinfo);

    /**
     * @brief Assign a random color to each given nodecontainer.
     * @param anim pointer to animationinterface
     * @param nodelists vector of nodecontainers to color
     */
    static void AutoAssignColor(AnimationInterface *anim, std::vector<NodeContainer> nodelists);

    /**
     * @brief Assign a size to each given nodecontainer, this size difference depends on the amount of nodecontainers.
     * @param anim pointer to animationinterface
     * @param nodelists vector of nodecontainers to assign a size
     */
    static void AutoAssignSize(AnimationInterface *anim, std::vector<NodeContainer> nodelists);

    /**
     * @brief Installs the multi-rat file protocol on nodes with multi-rat clients in the given nodecontainer
     * @param manager helper for communication between protocols
     * @param nodes nodes to install on
     */
    static void InstallFileProtocols(Ptr<MultiRatFileProtocolHelper> manager, NodeContainer nodes);

    /**
     * @brief Create a pair of udp client and server
     * @param node1 node to install the client on
     * @param node2 node to install the server on
     * @param port port used on the server
     * @param maxpackets maximum amount of packets to send
     * @param interval interval between packets
     * @param size size of packet payload in bytes
     * @return 
     */
    static ApplicationContainer CreateUdpClientPair(Ptr<Node> node1, Ptr<Node> node2, uint16_t port, uint32_t maxpackets, Time interval, uint16_t size); 

};

} // namespace ns3

#endif // MULTI_RAT_UTILITY_H