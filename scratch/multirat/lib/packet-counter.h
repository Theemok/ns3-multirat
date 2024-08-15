#ifndef PACKET_COUNTER_H
#define PACKET_COUNTER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <map>

namespace ns3 {

class Packet;
class Ipv4Address;
class Address;

/**
 * @brief Information about the connection between a udp client and its server
 */
struct ConnectionInfo 
{
    uint32_t sent; // amount of packets sent by the client
    uint32_t received; // amount of packets received by the server
    float time; // average time a packet takes to arrive at the server
    std::map<std::vector<std::tuple<uint32_t, uint32_t>>, uint32_t> routes; // count routes used by the network to deliver the packet;
    bool mishap; // true if last packet was not counted for average time due to being very out of line
};

/**
 * \ingroup MultiRat
 * @brief Header used to store information required for multi-rat routing
 */
class PacketCounter : public Object {
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Receives packet sent callback from udpclients and adds one to the sent counter for the given id
     * @param id id to increase the counter of
     * @param p packet sent
     */
    void HandleTX(std::string id, Ptr<Packet const> p);

    /**
     * @brief Receives packet received callback from udpservers and adds one to the received counter for the given id
     * @param id id to increase the counter of
     * @param p packet received
     */
    void
    HandleRX(std::string id, Ptr<Packet const> p);

    /**
     * @brief Calculates the final result of the ratio of succesful packet transmissions versus the total packets sent
     * @return percentage result of messages received versus total sent
     */
    std::tuple<float, uint32_t> GetResult();

    std::map<uint32_t, ConnectionInfo> info; //used to track successful packets sent and received by udpclients and udpservers

};

} // namespace ns3

#endif // PACKET_COUNTER_H