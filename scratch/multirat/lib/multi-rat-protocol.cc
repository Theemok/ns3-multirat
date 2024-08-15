
#include "multi-rat-protocol.h"

#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiRatProtocol");

NS_OBJECT_ENSURE_REGISTERED(MultiRatProtocol);

TypeId
MultiRatProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiRatProtocol")
            .SetParent<Object>()
            .SetGroupName("MultiRat");
    return tid;
}

void
MultiRatProtocol::ReceivePacket(Ptr<Packet const> p) {}

Ipv4Address
MultiRatProtocol::GetRoute(uint32_t node) 
{
    return Ipv4Address();
}

bool 
MultiRatProtocol::CheckKnownMultiRatClient(Address address) {
    return false;
}

std::vector<uint32_t>
MultiRatProtocol::GetCompatibleMultiRatClients(uint32_t channelid) {
    std::vector<uint32_t> vec;
    return vec;
}

uint32_t
MultiRatProtocol::GetNodeId(Ipv4Address address) 
{
    return 0;
}

Mac48Address
MultiRatProtocol::GetHardwareAddress(Ipv4Address address) 
{
    return Mac48Address();
}

bool 
MultiRatProtocol::ShouldSendArp(uint32_t node, Ptr<NetDevice> srcinterface) 
{
    return false;
}

void 
MultiRatProtocol::StartProtocol() {}

void 
MultiRatProtocol::StopProtocol() {}

void 
MultiRatProtocol::SetClient(Ptr<MultiRatClient> client) 
{
    m_client = client;
}

Ptr<MultiRatClient> 
MultiRatProtocol::GetClient()
{
    return m_client;
}

} // namespace ns3