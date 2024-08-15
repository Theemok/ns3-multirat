#ifndef MULTI_RAT_CLIENT_H
#define MULTI_RAT_CLIENT_H


#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "multi-rat-protocol.h"

#include <map>

namespace ns3
{

class Ipv4Interface;
class Socket;
class ArpHeader;
class Ipv4Header;
class ArpCache;
class MultiRatTag;
class MultiRatHeader;
class Ipv4;
class MultiRatProtocol;
class WifiNetDevice;

/**
 * \ingroup MultiRat
 * @brief A multi-rat enabling client
 */
class MultiRatClient : public Application
{
  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    MultiRatClient();

    ~MultiRatClient() override;

    /**
     * @brief Sets the routing protocol used by the MultiRatClient
     * @param protocol protocol to be used
     */
    void SetRoutingProtocol(Ptr<MultiRatProtocol> protocol);

    /**
     * @brief Converts a node id into a mac48address to store inside of an arpcache as a resolved destination
     * @param id node id to convert
     * @return converted node id
     */
    static Mac48Address NodeIdToMac(uint32_t id);

    /**
     * @brief Converts a stored mac48address from an arp cache to the corresponding node id
     * @param address address to convert
     * @return converted address
     */
    static uint32_t MacToNodeId(Mac48Address address);

    /**
     * @brief Sends an ipv4 packet to a custom destination
     * @param p packet to send
     * @param hdr ipv4 header to add to the packet
     * @param dest destination address for packet
     */
    void Send(Ptr<Packet> p, const Ipv4Header& hdr, Ipv4Address dest);

    /**
     * @brief Collect the channel ids of the meshpointdevices on the node
     * @return list of channel ids
     */
    std::vector<uint32_t> GetChannels();

    /**
     * @brief Get the meshpointdevice assigned to the given ip
     * @param dest destination ip address
     * @return pointer to assigned netdevice
     */
    Ptr<NetDevice> GetNetDeviceForDest(Ipv4Address dest);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;
    
    /**
     * @brief Connected to the phy of the interfaces of the MeshPointDevices of the node and fire when a packet is received
     * @param p the received packet
     */
    void ReceivePhy(Ptr<Packet const> p);

    /**
     * @brief Connected to the mac of the interfaces of the MeshPointDevices of the node and fire when a packet is received
     * @param p the received packet
     */
    void ReceiveArp(Ptr<Packet const> p);

    /**
     * @brief Connected to the Ipv4 of the node and fires when a packet is received
     * @param p the received packet
     * @param ipv4 Ipv4
     * @param interfaceIndex interface index that received the packet
     */
    void ReceivePacket(Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint32_t interfaceIndex);

    /**
     * @brief Sends a request to all compatible multi-rat nodes to request for a destination
     * @param arp arp header of the original packet
     */
    void SendArpRequests(ArpHeader arp);

    /**
     * @brief Forwards a request from another multi-rat node
     * @param arp arp header of the multi-rat packet
     * @param mrhead multi-rat header of the multi-rat packet
     */
    void ForwardArpRequest(ArpHeader arp, MultiRatHeader mrhead);

    /**
     * @brief Delivers a request from another multi-rat node
     * @param arp arp header of the multi-rat packet
     * @param mrhead multi-rat header of the multi-rat packet
     */
    void DeliverArpRequest(ArpHeader arp, MultiRatHeader mrhead);

    /**
     * @brief Check if an arp reply was requested by this node and send the reply back to the requester if so
     * @param arp arp header of the original packet
     */
    void SendArpReplies(ArpHeader arp);

    /**
     * @brief Forwards a reply from another multi-rat node
     * @param arp arp header of the multi-rat packet
     * @param mrhead multi-rat header of the multi-rat packet
     */
    void ForwardArpReply(ArpHeader arp, MultiRatHeader mrhead);

    /**
     * @brief Delivers a request from another multi-rat node
     * @param arp arp header of the multi-rat packet
     * @param mrhead multi-rat header of the multi-rat packet
     */
    void DeliverArpReply(ArpHeader arp, MultiRatHeader mrhead);

    /**
     * @brief Gets the channel id that would be used if a packet with given address is sent
     * @param  address given address
     * @return channel id
     */
    uint32_t GetChannelByIp(Ipv4Address address);

    Ptr<MultiRatProtocol> m_protocol; // Protocol used by MultiRatClient
    Ptr<ArpCache> m_cache; // Cache for resolved destinations
    Ptr<ArpCache> m_disccache; // Cache for remembering discovery packets

};

} // namespace ns3

#endif // MULTI_RAT_CLIENT_H
