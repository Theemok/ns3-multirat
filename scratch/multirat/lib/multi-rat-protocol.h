#ifndef MULTI_RAT_PROTOCOL_H
#define MULTI_RAT_PROTOCOL_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "multi-rat-client.h"

namespace ns3 {

class Mac48Address;
class Ipv4Address;
class Packet;
class MultiRatClient;

/**
 * \ingroup MultiRat
 * @brief Base proactive multi-rat protocol, meant to be inherited by other classes,
 * so they can be used by the multi-rat client
 */
class MultiRatProtocol : public Object 
{
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Set the client of the protocol
     * @param client pointer to client
     */
    void SetClient(Ptr<MultiRatClient> client);

    /**
     * @brief Get the client of the protocol
     * @return pointer to client
     */
    Ptr<MultiRatClient> GetClient();

    /**
     * @brief Called by client whenever a phy packet is received
     * @param p pointer to received packet
     */
    virtual void ReceivePacket(Ptr<Packet const> p);

    /**
     * @brief Get next hop to given node id
     * @param node destination node id
     * @return next hop address
     */
    virtual Ipv4Address GetRoute(uint32_t node);

    /**
     * @brief Get node id associated with an address
     * @param address address to inspect
     * @return associated node id
     */
    virtual uint32_t GetNodeId(Ipv4Address address);

    /**
     * @brief Get hardware address associated with address 
     * @param address assress to inspect
     * @return associated hardware address
     */
    virtual Mac48Address GetHardwareAddress(Ipv4Address address);

    /**
     * @brief Determines whether an arp request should be sent to this node
     * @param  node id we want to check
     * @param  srcinterface Netdevice the packet was received on
     * @return whether an arp request should be sent
     */
    virtual bool ShouldSendArp(uint32_t node, Ptr<NetDevice> srcinterface);

    /**
     * @brief Start the routing protocol
     */
    virtual void StartProtocol();

    /**
     * @brief Stop the routing protocol
     */
    virtual void StopProtocol();

    /**
     * @brief Check if address belongs to node with a multi rat client
     * @param address address to inspect
     * @return if address is known
     */
    virtual bool CheckKnownMultiRatClient(Address address);

    /**
     * @brief Get node ids of clients that are compatible with this channel id
     * @param channelid channel id
     * @return list of compatible node ids
     */
    virtual std::vector<uint32_t> GetCompatibleMultiRatClients(uint32_t channelid);

    private:

    Ptr<MultiRatClient> m_client; //Multi-rat client associated with this protocol

};

} // namespace ns3

#endif // MULTI_RAT_PROTOCOL_H