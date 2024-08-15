#include <iostream>

#include "multi-rat-tag.h"

#include "ns3/ipv4-address.h"
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/core-module.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("MultiRatTag");
  
  NS_OBJECT_ENSURE_REGISTERED (MultiRatTag);

  TypeId
  MultiRatTag::GetTypeId()
  {
      static TypeId tid = TypeId("ns3::MultiRatTag")
                              .SetParent<Tag>()
                              .SetGroupName("MultiRat")
                              .AddConstructor<MultiRatTag>();
      return tid;
  }

  TypeId MultiRatTag::GetInstanceTypeId() const {
      return GetTypeId();
  }

  uint32_t MultiRatTag::GetSerializedSize() const {
      return 8 + 4 + m_route.size() * 8;
  }

  void MultiRatTag::Serialize(TagBuffer i) const {
    i.WriteU64(m_time);
    i.WriteU32(m_route.size());
    for (auto info = m_route.begin(); info != m_route.end(); info++) {
      i.WriteU32(std::get<0>(*info));
      i.WriteU32(std::get<1>(*info));
    }
  }


  void
  MultiRatTag::Deserialize(TagBuffer i) {

    m_time = i.ReadU64();
    uint32_t entries = i.ReadU32();
    for (uint32_t entry = 0; entry < entries; entry++) {
      std::tuple<uint32_t, uint32_t> info;
      std::get<0>(info) = i.ReadU32();
      std::get<1>(info) = i.ReadU32();
      m_route.push_back(info);
    }
  }

  void
  MultiRatTag::Print(std::ostream& os) const {
    os << " ";
  }

void 
MultiRatTag::MarkTime()
{
  NS_LOG_FUNCTION(this);
  Time timestamp = Simulator::Now();
  m_time = timestamp.GetMicroSeconds();
}

uint64_t 
MultiRatTag::GetTime()
{
  NS_LOG_FUNCTION(this);
  return m_time;
}

void 
MultiRatTag::UpdateRoute(uint32_t nodeid, uint32_t channelid)
{
  std::tuple<uint32_t, uint32_t> info;
  std::get<0>(info) = nodeid;
  std::get<1>(info) = channelid;
  m_route.push_back(info);
}

std::vector<std::tuple<uint32_t, uint32_t>>
MultiRatTag::GetRoute()
{
  return m_route;
}

}