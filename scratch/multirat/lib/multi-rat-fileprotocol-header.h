#ifndef MULTI_RAT_FILE_PROTOCOL_HEADER_H
#define MULTI_RAT_FILE_PROTOCOL_HEADER_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"


namespace ns3 {

/**
 * \ingroup MultiRat
 * @brief Header for the multi-rat file protocol. Carries information required to route between different networks
 */
class MultiRatFileProtocolHeader : public Header {
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Sets the source id of the ping packet
     * @param node id of the node
     */
    void SetSourceId(uint32_t node);
    
    /**
     * @brief Gets the source id of the ping packet
     * @return id of the node
     */
    uint32_t GetSourceId();

    /**
     * @brief Marks the current simulator time
     */
    void MarkTime();

    /**
     * @brief Gets the marked time from the header
     * @return marked time
     */
    uint64_t GetTime();

    // Inherited from Header class:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    private:

    uint64_t m_time; // Marked time
    uint32_t m_sourceid; // Source id of the ping packet

};

} // namespace ns3

#endif // MULTI_RAT_FILE_PROTOCOL_HEADER_H