#ifndef MULTI_RAT_HEADER_H
#define MULTI_RAT_HEADER_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"

namespace ns3 {

/**
 * \ingroup MultiRat
 * @brief Header used to store information required for multi-rat routing
 */
class MultiRatHeader : public Header {
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Set the destination node id of the header
     * @param dest destination node id
     */
    void SetDestination(uint32_t dest);

    /**
     * @brief Get the destination node id of the header
     * @return destination node id
     */
    uint32_t GetDestination();

    /**
     * @brief Set the source node id of the header
     * @param src source node id
     */
    void SetSource(uint32_t src);

    /**
     * @brief Get the source node id of the header
     * @return source node id
     */
    uint32_t GetSource();

    // Inherited from Header class:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    private:

    uint32_t m_source; // Source node id
    uint32_t m_destination; // Destination node id

};

} // namespace ns3

#endif // MULTI_RAT_HEADER_H