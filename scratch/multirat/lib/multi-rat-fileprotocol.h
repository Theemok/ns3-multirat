#ifndef MULTI_RAT_FILEPROTOCOL_H
#define MULTI_RAT_FILEPROTOCOL_H

#include "multi-rat-protocol.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"

#include <map>

namespace ns3 {

class Mac48Address;
class Ipv4Address;
class Packet;

/**
 * @brief Data saved on nodes about their neighbours
 */
struct NeighbourData
{
    uint32_t node;
    Time lastseen;
    Mac48Address hardwareaddress;
    uint8_t hops;
    uint32_t deliverytime;
    std::string datarate;
    double reliability;
    uint32_t channelid;
};

/**
 * \ingroup MultiRat
 * @brief Basic protocol that writes neighbour information to a file and 
 * expects another application to compute its routes
 */
class MultiRatFileProtocol : public MultiRatProtocol 
{
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    MultiRatFileProtocol();

    ~MultiRatFileProtocol() override;

    /**
     * @brief Set the routes of the protocol
     * @param routes map of node ids with their associated next hop address
     */
    void SetRoutes(std::map<uint32_t, Ipv4Address> routes);

    /**
     * @brief Get all collected neighbour data
     * @return map of address with their associated neighbour data
     */
    std::map<Ipv4Address, NeighbourData> GetNeighbours();

    /**
     * @brief Set the compatibility map of the protocol
     * @param compatabilitymap map of channel ids and their associated list of node ids
     */
    void SetCompatibleMultiRatClients(std::map<uint32_t, std::vector<uint32_t>> compatabilitymap);

    /**
     * @brief Periodically sends a ping packet on every compatible channel
     */
    void PingNeighbours();

    // Inherited from MultiRatProtocol class:
    void ReceivePacket(Ptr<Packet const> p) override;
    Ipv4Address GetRoute(uint32_t node) override;
    uint32_t GetNodeId(Ipv4Address address) override;
    Mac48Address GetHardwareAddress(Ipv4Address address) override;
    bool CheckKnownMultiRatClient(Address address) override;
    std::vector<uint32_t> GetCompatibleMultiRatClients(uint32_t channelid) override;
    bool ShouldSendArp(uint32_t node, Ptr<NetDevice> srcinterface) override;
    void StartProtocol();
    void StopProtocol();

    private:

    EventId m_pingEvent; // Periodic update event for pings
    std::map<Ipv4Address, NeighbourData> m_neighbourdata; // Collected neighbour data
    std::map<uint32_t, Ipv4Address> m_routes; // Map of node ids and their associated next hop address
    std::map<uint32_t, std::vector<uint32_t>> m_compatabilitymap; // Map of compatible node ids
};

} // namespace ns3

#endif // MULTI_RAT_PROTOCOL_H