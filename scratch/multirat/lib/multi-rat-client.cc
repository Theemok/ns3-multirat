#include <map>

#include "multi-rat-client.h"
#include "multi-rat-tag.h"
#include "multi-rat-header.h"

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/hwmp-protocol.h"
#include "ns3/hwmp-rtable.h"
#include "ns3/ie-dot11s-preq.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiRatClient");

NS_OBJECT_ENSURE_REGISTERED(MultiRatClient);

TypeId
MultiRatClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiRatClient")
            .SetParent<Application>()
            .SetGroupName("MultiRat")
            .AddConstructor<MultiRatClient>();
    return tid;
}

MultiRatClient::MultiRatClient()
{
  NS_LOG_FUNCTION(this);
}

MultiRatClient::~MultiRatClient()
{
  NS_LOG_FUNCTION(this);
  m_protocol = nullptr;
  m_cache = nullptr;
  m_disccache = nullptr;
}

void
MultiRatClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
MultiRatClient::ReceivePhy(Ptr<Packet const> p) 
{
  NS_LOG_FUNCTION(this << p);
  m_protocol->ReceivePacket(p);
}

void
MultiRatClient::ReceiveArp(Ptr<Packet const> p) 
{
  NS_LOG_FUNCTION(this << p);
  Ptr<Packet> packet = p->Copy();
  LlcSnapHeader llc;
  MultiRatTag mrtag;
  //Check if the packet has the correct llc type
  if (packet->PeekHeader(llc)) {
    packet->RemoveHeader(llc);
    if (llc.GetType() == 2054) {
      
      //Ensure packet has arp header
      ArpHeader arp;
      if (packet->PeekHeader(arp)) {
        packet->RemoveHeader(arp);

        //Check if there is a multi-rat header
        //This means the packet comes from another multi-rat node and needs to be forwarded or delivered
        MultiRatHeader mrhead;
        if (packet->GetSize() == mrhead.GetSerializedSize() && packet->PeekHeader(mrhead)) {

          //Check if packet is a request
          if (arp.IsRequest()) {

            //If the packet is a request the destination node is given in the arp header itself
            if (MacToNodeId(Mac48Address::ConvertFrom(arp.GetDestinationHardwareAddress())) == GetNode()->GetId()) {
              if (mrhead.GetDestination() == GetNode()->GetId()) {
                //This node is the final destination of the request packet
                //Deliver the request packet
                DeliverArpRequest(arp, mrhead);
              } else {
                //This node is not the final destination of the request packet
                //Forward the request packet
                ForwardArpRequest(arp, mrhead);
              }
            } 

          //Check if packet is a reply
          } else if (arp.IsReply()) {
            if (mrhead.GetDestination() == GetNode()->GetId()) {
                //This node is the final destination of the request packet
                //Deliver the request packet
                DeliverArpReply(arp, mrhead);
              } else {
                //This node is not the final destination of the request packet
                //Forward the request packet
                ForwardArpReply(arp, mrhead);
              }
          }
        } else if (packet->GetSize() == 0) {
          //Packet does not come from a multi-rat node
          //Check if packet is a request
          if (arp.IsRequest()) {
            //Ensure the packet does not come from a known multi-rat node
            bool known = m_protocol->CheckKnownMultiRatClient(arp.GetSourceHardwareAddress());

            //Ensure this packet is not one we already initiated a search for
            ArpCache::Entry* entry = m_disccache->Lookup(arp.GetDestinationIpv4Address());
            if (!known && (entry == nullptr || !entry->IsWaitReply())) {
              SendArpRequests(arp);
            }


          } else if (arp.IsReply())
            //Packet is a reply from a non multi-rat node
            SendArpReplies(arp);
        }
      }
    }
  }
}

void
MultiRatClient::ReceivePacket(Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint32_t interfaceIndex)
{
  NS_LOG_FUNCTION(this << p << ipv4 << interfaceIndex);
  Ptr<Packet> packetCopy = p->Copy();

  Ipv4Header ipv4Header;
  packetCopy->RemoveHeader(ipv4Header);

  //If packet has a multi-rat header it is sent by a multi-rat node
  MultiRatHeader mrhead;
  if (ipv4Header.GetPayloadSize() + mrhead.GetSerializedSize() == packetCopy->GetSize()) {
    packetCopy->PeekHeader(mrhead);

    //If this is the destination node id the multi-rat header is removed and the packet is sent to its final destination
    //Otherwise it is forwarded to the next multi-rat node
    if (mrhead.GetDestination() == GetNode()->GetId()) {

      //If MultiRatTag is present we update its route to include this node id and the channel it is sent on next
      MultiRatTag mrtag;
      if (p->PeekPacketTag(mrtag)) {
        //Find out which channel the packet came from
        mrtag.UpdateRoute(GetNode()->GetId(), GetChannelByIp(ipv4Header.GetDestination()));
        packetCopy->ReplacePacketTag(mrtag);
      }

      NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " delivering ipv4 packet to " << ipv4Header.GetDestination());
      packetCopy->RemoveHeader(mrhead);
      Send(packetCopy, ipv4Header, ipv4Header.GetDestination());
    } else {

      //If MultiRatTag is present we update its route to include this node id and the channel it is sent on next
      MultiRatTag mrtag;
      if (p->PeekPacketTag(mrtag)) {
        //Find out which channel the packet came from
        mrtag.UpdateRoute(GetNode()->GetId(), GetChannelByIp(m_protocol->GetRoute(mrhead.GetDestination())));
        packetCopy->ReplacePacketTag(mrtag);
      }

      NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " forwarding ipv4 packet to be delivered to " << ipv4Header.GetDestination());
      Send(packetCopy, ipv4Header, m_protocol->GetRoute(mrhead.GetDestination()));
    }

  } else {
    //Not sent by a multi-rat node

    //If we have an alive entry for the destination the packet is sent over the multi-rat network
    ArpCache::Entry* entry = m_cache->Lookup(ipv4Header.GetDestination());
    if (entry != nullptr)
    {
      //The destination node id is saved as a mac address in the arpcache
      Mac48Address dest = Mac48Address::ConvertFrom(entry->GetMacAddress());
      uint32_t destid = MacToNodeId(dest);

      //Add multi-rat header
      MultiRatHeader mrhead;
      mrhead.SetSource(GetNode()->GetId());
      mrhead.SetDestination(destid);
      packetCopy->AddHeader(mrhead);

      //If MultiRatTag is present we update its route to include this node id and the channel it is sent on next
      MultiRatTag mrtag;
      if (p->PeekPacketTag(mrtag)) {
        //Find out which channel the packet came from
        mrtag.UpdateRoute(GetNode()->GetId(), GetChannelByIp(m_protocol->GetRoute(destid)));
        packetCopy->ReplacePacketTag(mrtag);
      }
      NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " received ipv4 packet to be delivered to " << ipv4Header.GetDestination());
      Send(packetCopy, ipv4Header, m_protocol->GetRoute(destid));
    }
  }
}

void
MultiRatClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    //For custom forwarding we connect ReceivePacket to the ipv4 RX event
    Ptr<Ipv4L3Protocol> ipv4 = m_node->GetObject<Ipv4L3Protocol>();
    ipv4->TraceConnectWithoutContext("Rx", MakeCallback(&MultiRatClient::ReceivePacket, this));

    for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++) {
      ipv4->SetForwarding(i, 0);
    }

    //Initialize arp caches and ensure they do not attempt a retry as they are just for storing packets
    m_cache = CreateObject<ArpCache>();
    m_cache->SetAttribute("MaxRetries", UintegerValue(0));
    m_cache->SetAliveTimeout(Seconds(5));

    m_disccache = CreateObject<ArpCache>();
    m_disccache->SetAttribute("MaxRetries", UintegerValue(0));

    //For receiving arp messages we connect to the Mac RX event of every interface of our meshpoint devices
    //The Phy RX event is required for routing information in the mesh header.
    for(uint8_t i = 0; i < (GetNode()->GetNDevices()); i++) {
      Ptr<MeshPointDevice> meshpoint = DynamicCast<MeshPointDevice>(GetNode()->GetDevice(i));
      if (meshpoint) {
        Ptr<dot11s::HwmpProtocol> hwmp = DynamicCast<dot11s::HwmpProtocol>(meshpoint->GetRoutingProtocol());
        if (hwmp) {
          hwmp->SetAttribute("DoFlag", BooleanValue(true));
        }

        std::vector<Ptr<NetDevice>> devlist = meshpoint->GetInterfaces();
        for (std::vector<Ptr<NetDevice>>::iterator it = devlist.begin(); it < devlist.end(); it++) {
          Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(*it);
          wifi->GetMac()->TraceConnectWithoutContext("MacRx", MakeCallback(&MultiRatClient::ReceiveArp, this));
          wifi->GetPhy()->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&MultiRatClient::ReceivePhy, this));
        }
      }
    }

    m_protocol->StartProtocol();
}

void
MultiRatClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    m_protocol->StopProtocol();
    m_cache->Dispose();
    m_disccache->Dispose();
}

Mac48Address MultiRatClient::NodeIdToMac(uint32_t id)
{
  // Fill a buffer with the id and create a mac48address using it
  uint8_t buffer[6];
  buffer[2] = (id &0xff000000) >> 24;
  buffer[3] = (id &0x00ff0000) >> 16;
  buffer[4] = (id &0x0000ff00) >> 8;
  buffer[5] = (id &0x000000ff);
  Mac48Address address;
  address.CopyFrom(buffer);
  return address;
}

uint32_t MultiRatClient::MacToNodeId(Mac48Address address)
{
  // Convert the address to a buffer and fill a uin32_t with it
  uint32_t nodeid;
  uint8_t buffer[6];
  address.CopyTo(buffer);
  nodeid = buffer[2] << 24;
  nodeid += buffer[3] << 16;
  nodeid += buffer[4] << 8;
  nodeid += buffer[5];
  return nodeid;
}

void
MultiRatClient::SendArpRequests(ArpHeader arp)
{
  NS_LOG_FUNCTION(this << arp);

  Ptr<Packet> fakepacket = Create<Packet>();
  Ipv4Header ipv4;
  ArpCache::Entry* entry = m_cache->Lookup(arp.GetDestinationIpv4Address());
  if (entry == nullptr) {
    entry = m_cache->Add(arp.GetDestinationIpv4Address());
    entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(fakepacket, ipv4));
    entry->SetMacAddress(NodeIdToMac(0xFFFFFFFF));
  } else {
    if (entry->IsWaitReply()) {
      entry->UpdateWaitReply(ArpCache::Ipv4PayloadHeaderPair(fakepacket, ipv4));
    } else {
      entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(fakepacket, ipv4));
    }
  }

  //Find out which channel the packet came from
  Ptr<NetDevice> srcinterface = GetNetDeviceForDest(arp.GetSourceIpv4Address());
  Ptr<MeshPointDevice> meshpoint = DynamicCast<MeshPointDevice>(srcinterface);
  Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(meshpoint  ->GetInterfaces()[0]);
  uint32_t channelid = wifi->GetChannel()->GetId();

  //Loop over all known nodes compatible with the channel
  uint32_t sendcounter = 0;
  std::vector<uint32_t> compatibleclients = m_protocol->GetCompatibleMultiRatClients(channelid);
  for(std::vector<uint32_t>::iterator node = compatibleclients.begin(); node != compatibleclients.end(); node++) {
    //Check with protocol if we want to send an arp request to this node
    if (m_protocol->ShouldSendArp(*node, srcinterface)) {
      Ptr<Packet> packet = Create<Packet>();

      //Add multi-rat header
      MultiRatHeader mrhead;
      mrhead.SetSource(GetNode()->GetId());
      mrhead.SetDestination(*node);
      packet->AddHeader(mrhead);

      //Add arp header
      uint32_t dest = m_protocol->GetNodeId(m_protocol->GetRoute(*node));
      arp.SetRequest(arp.GetSourceHardwareAddress(), arp.GetSourceIpv4Address(), NodeIdToMac(dest), arp.GetDestinationIpv4Address());
      packet->AddHeader(arp);

      Ptr<NetDevice> dev = GetNetDeviceForDest(m_protocol->GetRoute(*node));
      //Send packet with delay to prevent collisions 
      NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " sending out ARP request for " << arp.GetDestinationIpv4Address() << " to node " << (*node));
      Simulator::Schedule(MicroSeconds(0),
                          &NetDevice::Send,
                          dev,
                          packet,
                          dev->GetBroadcast(),
                          ArpL3Protocol::PROT_NUMBER
                          );

      sendcounter++;
    }
  }
}

void
MultiRatClient::ForwardArpRequest(ArpHeader arp, MultiRatHeader mrhead)
{
  NS_LOG_FUNCTION(this << arp << mrhead);

  //Don't attempt to forward if device does not exist
  Ptr<NetDevice> dev = GetNetDeviceForDest(m_protocol->GetRoute(mrhead.GetDestination()));
  if (dev) {
    Ptr<Packet> packet = Create<Packet>();

    //Add multi-rat header
    packet->AddHeader(mrhead);

    //Add arp header
    uint32_t dest = m_protocol->GetNodeId(m_protocol->GetRoute(mrhead.GetDestination()));
    arp.SetRequest(arp.GetSourceHardwareAddress(), arp.GetSourceIpv4Address(), NodeIdToMac(dest), arp.GetDestinationIpv4Address());
    packet->AddHeader(arp);

    //Send packet
    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " forwarding ARP request for node " << mrhead.GetDestination() << " from node " << mrhead.GetSource() << " to " << m_protocol->GetRoute(mrhead.GetDestination()));
    dev->Send(packet, dev->GetBroadcast(), ArpL3Protocol::PROT_NUMBER);
  }
}

void
MultiRatClient::DeliverArpRequest(ArpHeader arp, MultiRatHeader mrhead) {
  NS_LOG_FUNCTION(this << arp << mrhead);

  //Do not deliver an arp that you are currently looking for as it is likely a longer route
  ArpCache::Entry* entry = m_cache->Lookup(arp.GetDestinationIpv4Address());
  if (!entry || !entry->IsWaitReply()) {

    //Check if entry exists and react accordingly in case it does
    ArpCache::Entry* entry = m_disccache->Lookup(arp.GetDestinationIpv4Address());
    if (entry != nullptr) {
      if (!entry->IsExpired()) {
        if (entry->IsDead()) {
          //Entry is dead and has not expired yet
          //Drop arp request
          NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " found dead " << arp.GetDestinationIpv4Address() << " and dropping");
          return;
        }
      }
    } else {
      //Create entry for arp request
      entry = m_disccache->Add(arp.GetDestinationIpv4Address());
    }

    //Create packet to store in cache
    Ptr<Packet> cachepacket = Create<Packet>();
    cachepacket->AddHeader(mrhead);
    cachepacket->AddHeader(arp);
    Ipv4Header ipv4Header;

    //Don't attempt to deliver if device does not exist
    Ptr<NetDevice> dev = GetNetDeviceForDest(arp.GetDestinationIpv4Address());
    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " starting discovery for " << arp.GetDestinationIpv4Address());
    if (dev) {

      //Update entry if entry was already waiting
      if (!entry->IsWaitReply()) {
        entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(cachepacket, ipv4Header));
      } else {
        entry->UpdateWaitReply(ArpCache::Ipv4PayloadHeaderPair(cachepacket, ipv4Header));
      }

      //Get own ipv4 address
      Ptr<Ipv4L3Protocol> ipv4 = GetNode()->GetObject<Ipv4L3Protocol>();
      int32_t interfaceid = ipv4->GetInterfaceForDevice(dev);
      Ptr<Ipv4Interface> interface = ipv4->GetInterface(interfaceid);
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress(interfaceid,0);
      Ipv4Address source = iaddr.GetLocal();

      //Create new arp request for destination and send
      Ptr<Packet> packet = Create<Packet>();
      ArpHeader newarp;
      newarp.SetRequest(dev->GetAddress(), source, arp.GetDestinationHardwareAddress(), arp.GetDestinationIpv4Address());
      packet->AddHeader(newarp);

      dev->Send(packet, dev->GetBroadcast(), ArpL3Protocol::PROT_NUMBER);
    }
  }
}

void
MultiRatClient::SendArpReplies(ArpHeader arp)
{
  NS_LOG_FUNCTION(this << arp);

  //Check if this node has requested this destination
  ArpCache::Entry* entry = m_disccache->Lookup(arp.GetSourceIpv4Address());
  if (entry != nullptr)
  { 
    //Continue if we are waiting for one ore more recent requests to be fullfilled
    if (entry->IsWaitReply())
    {
      ArpCache::Ipv4PayloadHeaderPair pending = entry->DequeuePending();
      entry->MarkAlive(Mac48Address("00:00:00:00:00:00"));

      // loop over all requests for the destination and provide a response for each
      uint32_t sendcounter = 0;
      while (pending.first) {
        ArpHeader pendingarp;
        pending.first->RemoveHeader(pendingarp);
        MultiRatHeader pendingmrhead;
        pending.first->PeekHeader(pendingmrhead);

        //Create new packet
        Ptr<Packet> packet = Create<Packet>();

        //Add new multi-rat header
        pendingmrhead.SetDestination(pendingmrhead.GetSource());
        pendingmrhead.SetSource(GetNode()->GetId());
        packet->AddHeader(pendingmrhead);

        //Add arp header
        pendingarp.SetReply(arp.GetSourceHardwareAddress(), pendingarp.GetDestinationIpv4Address(), pendingarp.GetSourceHardwareAddress(), pendingarp.GetSourceIpv4Address());
        packet->AddHeader(pendingarp);

        Ipv4Address dest = m_protocol->GetRoute(pendingmrhead.GetDestination());
        Mac48Address hardwaredest = m_protocol->GetHardwareAddress(dest);
        Ptr<NetDevice> dev = GetNetDeviceForDest(dest);

        if (dev) {
          Ptr<UniformRandomVariable> ran = CreateObject<UniformRandomVariable>();
          ran->SetStream(1);
          NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " discovered " << arp.GetSourceIpv4Address() << " and reporting to node " << pendingmrhead.GetDestination() << " through " << dest);
          Simulator::Schedule(MicroSeconds(ran->GetInteger(0,200)),
                    &NetDevice::Send,
                    dev,
                    packet,
                    hardwaredest,
                    ArpL3Protocol::PROT_NUMBER
                    );

        sendcounter++;
        }

        pending = entry->DequeuePending();
      }
    }
  }
}

void
MultiRatClient::ForwardArpReply(ArpHeader arp, MultiRatHeader mrhead)
{
  NS_LOG_FUNCTION(this << arp << mrhead);

  //Don't attempt to forward if device does not exist
  Ptr<NetDevice> dev = GetNetDeviceForDest(m_protocol->GetRoute(mrhead.GetDestination()));
  if (dev) {
    Ptr<Packet> packet = Create<Packet>();

    //Add multi-rat header
    packet->AddHeader(mrhead);

    //Add arp header
    packet->AddHeader(arp);

    //Send packet
    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " forwarding ARP reply for node " << mrhead.GetDestination() << " from node " << mrhead.GetSource() << " to " << m_protocol->GetRoute(mrhead.GetDestination()));
    Mac48Address dest = m_protocol->GetHardwareAddress(m_protocol->GetRoute(mrhead.GetDestination()));
    dev->Send(packet, dest, ArpL3Protocol::PROT_NUMBER);
  }
}

void
MultiRatClient::DeliverArpReply(ArpHeader arp, MultiRatHeader mrhead)
{
  NS_LOG_FUNCTION(this << arp << mrhead);

  //Don't attempt to deliver if device does not exist
  Ptr<NetDevice> dev = GetNetDeviceForDest(arp.GetDestinationIpv4Address());
  if (dev) {

    //Add location as entry to cache to be able to deliver packets later
    ArpCache::Entry* entry = m_cache->Lookup(arp.GetSourceIpv4Address());

    entry = m_cache->Lookup(arp.GetSourceIpv4Address());
    if (entry == nullptr || entry->IsExpired() || entry->IsWaitReply() || entry->IsDead()) {
      m_cache->Remove(entry);
      entry = m_cache->Add(arp.GetSourceIpv4Address());
      entry->UpdateSeen();
      uint8_t buffer[6];
      uint32_t destid = mrhead.GetSource();
      buffer[2] = (destid &0xff000000) >> 24;
      buffer[3] = (destid &0x00ff0000) >> 16;
      buffer[4] = (destid &0x0000ff00) >> 8;
      buffer[5] = (destid &0x000000ff);
      Mac48Address dest;
      dest.CopyFrom(buffer);
      entry->SetMacAddress(dest);
    }

    //Create new packet
    Ptr<Packet> packet = Create<Packet>();

    //Add arp
    arp.SetReply(dev->GetAddress(), arp.GetSourceIpv4Address(), arp.GetDestinationHardwareAddress(), arp.GetDestinationIpv4Address());
    packet->AddHeader(arp);

    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " delivering ARP reply to " << arp.GetDestinationIpv4Address());
    dev->Send(packet, arp.GetDestinationHardwareAddress(), ArpL3Protocol::PROT_NUMBER);
  }
}

void
MultiRatClient::Send(Ptr<Packet> p, const Ipv4Header& hdr, Ipv4Address dest)
{
  NS_LOG_FUNCTION(this << p << hdr << dest);

  // an Ipv4Interface is required to send a packet with a header to a custom destination
  Ptr<NetDevice> dev = GetNetDeviceForDest(dest);
  if (dev) {
    // extract the interface used to send packets to destination
    Ptr<Ipv4L3Protocol> ipv4 = GetNode()->GetObject<Ipv4L3Protocol>();
    uint32_t interfaceid = ipv4->GetInterfaceForDevice(dev);
    Ptr<Ipv4Interface> interface = ipv4->GetInterface(interfaceid);

    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " sending packet to " << dest);
    interface->Send(p, hdr, dest);
  }
}

Ptr<NetDevice>
MultiRatClient::GetNetDeviceForDest(Ipv4Address dest)
{
  NS_LOG_FUNCTION(this << dest);

  // extract the route for a given destination
  Ptr<Ipv4L3Protocol> ipv4 = GetNode()->GetObject<Ipv4L3Protocol>();
  Socket::SocketErrno errno_ = Socket::ERROR_NOTERROR;
  Ptr<Packet> p = Create<Packet>();
  Ipv4Header routeipv4;
  routeipv4.SetDestination(dest);
  Ptr<Ipv4Route> route = ipv4->GetRoutingProtocol()->RouteOutput(p, routeipv4, 0, errno_);

  // ensure a route exists or return a nullptr
  if (!route) {
    NS_LOG_INFO("At " << Simulator::Now().As(Time::S) << " node " << GetNode()->GetId() << " failed to find a NetDevice for destination " << dest);
    return nullptr;
  }
  return route->GetOutputDevice();
}



std::vector<uint32_t>
MultiRatClient::GetChannels()
{
  std::vector<uint32_t> channels;
  for(uint8_t i = 0; i < (GetNode()->GetNDevices()); i++) {
    Ptr<MeshPointDevice> meshpoint = DynamicCast<MeshPointDevice>(GetNode()->GetDevice(i));
    if (meshpoint) {
      std::vector<Ptr<NetDevice>> devlist = meshpoint->GetInterfaces();
      Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(devlist[0]);
      uint32_t channelid = wifi->GetChannel()->GetId();
      channels.push_back(channelid);
    }
  }
  return channels;
}

void
MultiRatClient::SetRoutingProtocol(Ptr<MultiRatProtocol> protocol)
{
  m_protocol = protocol;
  m_protocol->SetClient(this);
}

uint32_t 
MultiRatClient::GetChannelByIp(Ipv4Address address) 
{
  NS_LOG_FUNCTION(this << address);
  Ptr<NetDevice> srcinterface = GetNetDeviceForDest(address);
  if (srcinterface) {
    Ptr<MeshPointDevice> meshpoint = DynamicCast<MeshPointDevice>(srcinterface);
    Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(meshpoint  ->GetInterfaces()[0]);
    return wifi->GetChannel()->GetId();
  }
  return 0;
}

}
