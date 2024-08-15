#ifndef MULTI_RAT_TAG_H
#define MULTI_RAT_TAG_H

#include "ns3/tag.h"
#include "ns3/ipv4-address.h"

namespace ns3
{


// define this class in a public header
class MultiRatTag : public Tag {
  public:

  static TypeId GetTypeId(void);
  virtual TypeId GetInstanceTypeId(void) const;
  virtual uint32_t GetSerializedSize(void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream & os) const;

  /**
   * @brief Marks the current simulator time
   */
  void MarkTime();

  /**
   * @brief Gets the marked time from the header
   * @return marked time
   */
  uint64_t GetTime();

  void UpdateRoute(uint32_t nodeid, uint32_t channelid);

  std::vector<std::tuple<uint32_t, uint32_t>> GetRoute();

  uint64_t m_time;
  std::vector<std::tuple<uint32_t, uint32_t>> m_route;

};

}
#endif /* MYTAG_H */
