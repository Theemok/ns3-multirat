
#include "multi-rat-header.h"

#include "ns3/address-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MultiRatHeader");

NS_OBJECT_ENSURE_REGISTERED(MultiRatHeader);

TypeId
MultiRatHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MultiRatHeader")
                            .SetParent<Header>()
                            .SetGroupName("MultiRat")
                            .AddConstructor<MultiRatHeader>();
    return tid;
}

TypeId
MultiRatHeader::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

void
MultiRatHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "source: " << m_source << " " << "destination: " << m_destination;
}

uint32_t
MultiRatHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4 + 4;
}

void
MultiRatHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    i.WriteU32(m_source);
    i.WriteU32(m_destination);
}

uint32_t
MultiRatHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    m_source = i.ReadU32();
    m_destination = i.ReadU32();

    return GetSerializedSize();
}

void MultiRatHeader::SetSource(uint32_t src)
{
    NS_LOG_FUNCTION(this << src);
    m_source = src;
}

uint32_t MultiRatHeader::GetSource()
{
    NS_LOG_FUNCTION(this);
    return m_source;
}

void MultiRatHeader::SetDestination(uint32_t dst)
{
    NS_LOG_FUNCTION(this << dst);
    m_destination = dst;
}

uint32_t MultiRatHeader::GetDestination()
{
    NS_LOG_FUNCTION(this);
    return m_destination;
}

} // namespace ns3