
#include "multi-rat-fileprotocol-header.h"

#include "multi-rat-client.h"

#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/core-module.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MultiRatFileProtocolHeader");

NS_OBJECT_ENSURE_REGISTERED(MultiRatFileProtocolHeader);

TypeId
MultiRatFileProtocolHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MultiRatFileProtocolHeader")
                            .SetParent<MultiRatProtocol>()
                            .SetGroupName("MultiRat")
                            .AddConstructor<MultiRatFileProtocolHeader>();
    return tid;
}

TypeId
MultiRatFileProtocolHeader::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

void
MultiRatFileProtocolHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "sourceid: " << m_sourceid << ", time: " << m_time;
}

uint32_t
MultiRatFileProtocolHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4 + 8;
}

void
MultiRatFileProtocolHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    i.WriteU32(m_sourceid);
    i.WriteU64(m_time);

}

uint32_t
MultiRatFileProtocolHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    
    m_sourceid = i.ReadU32();
    m_time = i.ReadU64();

    return GetSerializedSize();
}

void
MultiRatFileProtocolHeader::SetSourceId(uint32_t node)
{
    NS_LOG_FUNCTION(this << node);
    m_sourceid = node;
}
uint32_t
MultiRatFileProtocolHeader::GetSourceId()
{
    NS_LOG_FUNCTION(this);
    return m_sourceid;
}

void 
MultiRatFileProtocolHeader::MarkTime()
{
    NS_LOG_FUNCTION(this);
    Time timestamp = Simulator::Now();
    m_time = timestamp.GetMicroSeconds();
}

uint64_t 
MultiRatFileProtocolHeader::GetTime()
{
    NS_LOG_FUNCTION(this);
    return m_time;
}




} // namespace ns3