
#include "multi-rat-fileprotocol.h"

#include <algorithm>

#include "multi-rat-client.h"
#include "multi-rat-tag.h"
#include "multi-rat-fileprotocol-header.h"

#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"
#include "ns3/ptr.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/constant-rate-wifi-manager.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiRatFileProtocol");

NS_OBJECT_ENSURE_REGISTERED(MultiRatFileProtocol);

TypeId
MultiRatFileProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiRatFileProtocol")
            .SetParent<MultiRatProtocol>()
            .SetGroupName("MultiRat")
            .AddConstructor<MultiRatFileProtocol>();
    return tid;
}

MultiRatFileProtocol::MultiRatFileProtocol()
{
    NS_LOG_FUNCTION(this);
}

MultiRatFileProtocol::~MultiRatFileProtocol()
{
    NS_LOG_FUNCTION(this);
}

void 
MultiRatFileProtocol::StartProtocol() 
{
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    m_pingEvent = Simulator::Schedule(Seconds(x->GetValue(0,5)), &MultiRatFileProtocol::PingNeighbours, this);
}

void 
MultiRatFileProtocol::StopProtocol() 
{
    Simulator::Cancel(m_pingEvent);
}

void
MultiRatFileProtocol::ReceivePacket(Ptr<Packet const> p)
{
    //We are only interested in packets with qos data
    WifiMacHeader wifihead;
    if (p->PeekHeader(wifihead) && wifihead.IsQosData()) {

        //Remove the wifimacheader
        Ptr<Packet> packet = p->Copy();
        packet->RemoveHeader(wifihead);
        packet->RemoveAtEnd(4);

        //Remove the MeshHeader
        uint8_t buf[6];
        packet->CopyData(buf, 6);
        uint8_t hops = 33 - buf[1];
        packet->RemoveAtStart(6);

        //Check if llc header is correct type
        LlcSnapHeader llc;
        if (packet->PeekHeader(llc) && llc.GetType() == 0x806) {
            packet->RemoveHeader(llc);
            
            //Check and remove arp header
            ArpHeader arp;
            if (packet->PeekHeader(arp)) {
                packet->RemoveHeader(arp);

                //Check our fileprotocolheader
                MultiRatFileProtocolHeader filehead;
                if (packet->GetSize() == filehead.GetSerializedSize() && packet->PeekHeader(filehead)) {
                    
                    if (GetClient()->GetNode()->GetId() != filehead.GetSourceId()) {

                        //Don't accept updates after the first as multiple copies may arrive
                        bool copy = false;
                        if (m_neighbourdata.find(arp.GetSourceIpv4Address()) != m_neighbourdata.end()) {
                            if (m_neighbourdata[arp.GetSourceIpv4Address()].lastseen + Seconds(1) > Simulator::Now()) {
                                copy = true;
                            }
                        }

                        if (!copy) {
                            NeighbourData info;
                            
                            Ptr<MeshPointDevice> dev = DynamicCast<MeshPointDevice>(GetClient()->GetNetDeviceForDest(arp.GetDestinationIpv4Address()));
                            if (dev) {
                                //Set data rate
                                Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(dev->GetInterfaces()[0]);
                                Ptr<ConstantRateWifiManager> rsm = DynamicCast<ConstantRateWifiManager>(wifi->GetRemoteStationManager());
                                if (rsm) {
                                    StringValue mode;
                                    rsm->GetAttribute("DataMode", mode);
                                    info.datarate = mode.Get();
                                }

                                //Set channelid
                                info.channelid = wifi->GetChannel()->GetId();
                            }

                            //Find out if this address is a known multi-rat client
                            uint32_t time = Simulator::Now().GetMicroSeconds() - filehead.GetTime();
                            if (m_neighbourdata.find(arp.GetSourceIpv4Address()) != m_neighbourdata.end()) {
                                //Update delivery time
                                info.deliverytime = m_neighbourdata[arp.GetSourceIpv4Address()].deliverytime * 0.8 + (time * 0.2);

                                //Update reliability
                                uint32_t timesincelast = Simulator::Now().GetSeconds() -  m_neighbourdata[arp.GetSourceIpv4Address()].lastseen.GetSeconds();
                                info.reliability = m_neighbourdata[arp.GetSourceIpv4Address()].reliability * 0.8 + std::min(1.0 / (timesincelast / 5.0), 1.0) * 0.2;
                            } else {
                                //Set default values
                                info.deliverytime = time;
                                info.reliability = 1.0;
                            }

                            info.node = filehead.GetSourceId();
                            info.lastseen = Simulator::Now();
                            info.hardwareaddress = Mac48Address::ConvertFrom(arp.GetSourceHardwareAddress());
                            info.hops = hops;

                            m_neighbourdata[arp.GetSourceIpv4Address()] = info;
                        }
                    }
                }
            }       
        }
    }
}

void
MultiRatFileProtocol::PingNeighbours()
{
    NS_LOG_FUNCTION(this);

    //
    for (auto it = m_neighbourdata.begin(); it != m_neighbourdata.end(); it++) {
        if (Simulator::Now() - (*it).second.lastseen > Seconds(12)) {
        m_neighbourdata[(*it).first].reliability = std::max(m_neighbourdata[(*it).first].reliability - 0.1, 0.0);
        }
    }

    //Loop through all MeshPointDevices and send a ping packet on each
    for (uint32_t i = 0; i < GetClient()->GetNode()->GetNDevices(); i++) {
        Ptr<MeshPointDevice> dev = DynamicCast<MeshPointDevice>(GetClient()->GetNode()->GetDevice(i));
        if (dev) {

            //Get own ipv4 address
            Ptr<Ipv4L3Protocol> ipv4 = GetClient()->GetNode()->GetObject<Ipv4L3Protocol>();
            int32_t interfaceid = ipv4->GetInterfaceForDevice(dev);
            Ptr<Ipv4Interface> interface = ipv4->GetInterface(interfaceid);
            Ipv4InterfaceAddress iaddr = ipv4->GetAddress(interfaceid,0);
            Ipv4Address source = iaddr.GetLocal();

            //Create packet to ping other MultiRatClients
            Ptr<Packet> p = Create<Packet>();
            MultiRatTag mrtag;

            //Add fileprotocol header
            MultiRatFileProtocolHeader filehead;
            filehead.MarkTime();
            filehead.SetSourceId(GetClient()->GetNode()->GetId());
            p->AddHeader(filehead);

            //Add arp header
            ArpHeader arp;
            arp.SetRequest(dev->GetAddress(), source, dev->GetBroadcast(), iaddr.GetBroadcast());
            p->AddHeader(arp);

            dev->Send(p, dev->GetBroadcast(), ArpL3Protocol::PROT_NUMBER);
        }
    }

    // reschedule sending ping packets
    m_pingEvent = Simulator::Schedule(Seconds(5), &MultiRatFileProtocol::PingNeighbours, this);

}

bool 
MultiRatFileProtocol::CheckKnownMultiRatClient(Address address) {
    // Loop over neighbour data to see if any hardware address matches given address
    for(std::map<Ipv4Address, NeighbourData>::iterator ip = m_neighbourdata.begin(); ip != m_neighbourdata.end(); ++ip) {
        if (ip->second.hardwareaddress == address) {
            return true;
        }
    }
    return false;
}

std::vector<uint32_t> 
MultiRatFileProtocol::GetCompatibleMultiRatClients(uint32_t channelid) {
    if (m_compatabilitymap.find(channelid) != m_compatabilitymap.end()) {
        return m_compatabilitymap[channelid];
    }
    return std::vector<uint32_t>();
}

bool
MultiRatFileProtocol::ShouldSendArp(uint32_t node, Ptr<NetDevice> srcinterface)
{
    //Make sure we don't send an arp request to self
    if (GetClient()->GetNode()->GetId() != (node)) {
        Ipv4Address route = GetRoute(node);

        //Don't send arp request to nodes if their path starts off through the original channel or device doesnt exist
        Ptr<NetDevice> dev = GetClient()->GetNetDeviceForDest(route);
        if (dev && srcinterface != dev) {

        //Don't send arp requests that are too close
        NeighbourData data = m_neighbourdata[route];

        Ptr<MeshPointDevice> meshpoint = DynamicCast<MeshPointDevice>(srcinterface);
        Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(meshpoint->GetInterfaces()[0]);
        uint32_t channelid = wifi->GetChannel()->GetId();

        //Find route that would be used in the original network
        //Return false if original route does not gain much time from swapping channels
        //This only works if both routes directly lead to the destination node
        if (m_neighbourdata[route].node == node) {
            for (auto it = m_neighbourdata.begin(); it != m_neighbourdata.end(); it++) {
                if ((*it).second.node == node && (*it).second.channelid == channelid) {
                    if ((*it).second.deliverytime > m_neighbourdata[route].deliverytime + 400) {
                        return true;
                    } else {
                        return false;
                    }
                }
            }
        }
        return true;
      }
    }
    return false;
}

void 
MultiRatFileProtocol::SetCompatibleMultiRatClients(std::map<uint32_t, std::vector<uint32_t>> compatabilitylist) {
    m_compatabilitymap = compatabilitylist;
}

Ipv4Address
MultiRatFileProtocol::GetRoute(uint32_t node)
{
    NS_LOG_FUNCTION(this << node);
    if (m_routes.find(node) == m_routes.end()) {
        return Ipv4Address("255.255.255.255");
    }
    return m_routes[node];
}

uint32_t
MultiRatFileProtocol::GetNodeId(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);
    NS_ASSERT(m_neighbourdata.find(address) != m_neighbourdata.end());
    return m_neighbourdata[address].node;
}

Mac48Address
MultiRatFileProtocol::GetHardwareAddress(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);
    NS_ASSERT(m_neighbourdata.find(address) != m_neighbourdata.end());
    return m_neighbourdata[address].hardwareaddress;
}

void
MultiRatFileProtocol::SetRoutes(std::map<uint32_t, Ipv4Address> routes) {
  NS_LOG_FUNCTION(this);
  m_routes = routes;
}

std::map<Ipv4Address, NeighbourData>
MultiRatFileProtocol::GetNeighbours()
{
    return m_neighbourdata;
}

} // namespace ns3