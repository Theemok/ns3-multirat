
#include "packet-counter.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/address.h"
#include "multi-rat-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketCounter");

NS_OBJECT_ENSURE_REGISTERED(PacketCounter);

TypeId
PacketCounter::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PacketCounter")
            .SetParent<Object>()
            .SetGroupName("MultiRat")
            .AddConstructor<PacketCounter>();
    return tid;
}
void
PacketCounter::HandleTX(std::string id, Ptr<Packet const> p)
{
    uint32_t idnum = std::stoul(id);
    if (info.find(idnum) == info.end()) {
        ConnectionInfo con;
        con.sent = 0;
        con.received = 0;
        con.time = 0;
        con.routes = std::map<std::vector<std::tuple<uint32_t, uint32_t>>, uint32_t>();
        con.mishap = true;
        info[idnum] = con;
    }

    info[idnum].sent += 1; 

    MultiRatTag mrtag;
    mrtag.MarkTime();
    p->AddPacketTag(mrtag);
}

void
PacketCounter::HandleRX(std::string id, Ptr<Packet const> p)
{
    uint32_t idnum = std::stoul(id);
    NS_ASSERT(info.find(idnum) != info.end());

    MultiRatTag mrtag;
    if (p->PeekPacketTag(mrtag)) {
        if (info[idnum].received >= 5) {
           
            if ((info[idnum].time * 3 > Simulator::Now().GetMicroSeconds() - mrtag.GetTime()) || info[idnum].mishap) {
                //NS_LOG_UNCOND(info[idnum].time << " " << Simulator::Now().GetMicroSeconds() << " " << mrtag.GetTime() << " " << Simulator::Now().GetMicroSeconds() - mrtag.GetTime());
                info[idnum].time = (info[idnum].time * (float) (info[idnum].received - 5) + (float) (Simulator::Now().GetMicroSeconds() - mrtag.GetTime())) / (float) ( info[idnum].received - 4);
            }

            if (info[idnum].time * 3 > Simulator::Now().GetMicroSeconds() - mrtag.GetTime()) {
                info[idnum].mishap = false;
            } else {
                info[idnum].mishap = true;
            }
        }
        info[idnum].received += 1; 
        info[idnum].routes[mrtag.GetRoute()] += 1;
    }
}

std::tuple<float, uint32_t>
PacketCounter::GetResult()
{
    uint64_t totalsent = 0;
    uint64_t totalrecv = 0;
    uint64_t totaltime = 0;
    if (info.size() == 0) {
        std::tuple<float, uint32_t> result;
        std::get<0>(result) = 0;
        std::get<1>(result) = 0;
        return result;
    }
    for (std::map<uint32_t, ConnectionInfo>::iterator it = info.begin(); it != info.end(); it++) {
        totaltime += (*it).second.time;

        uint32_t sent = (*it).second.sent;
        totalsent += sent;

        uint32_t recv = (*it).second.received;
        totalrecv += recv;
        
        NS_LOG_INFO("udp pair id: " << (*it).first << ", sent: " << sent << ", received: " << recv << ", reliability: " << ((double)recv / (double)sent) << ", avg time in milli: " << (*it).second.time << ", routes used (nodeid, channelid used ->): ");
        
        uint64_t multiratrec = 0;
        for (std::map<std::vector<std::tuple<uint32_t, uint32_t>>, uint32_t>::iterator route = (*it).second.routes.begin(); route != (*it).second.routes.end(); route++) {
            if ((*route).first.size() == 0) {
                continue;
            }
            
            multiratrec += (*route).second;
            std::string routestr;
            std::string seperator;
            for (auto stop = (*route).first.begin(); stop != (*route).first.end(); stop++) {
                routestr += seperator + std::to_string(std::get<0>(*stop)) + ", " + std::to_string(std::get<1>(*stop));
                seperator = " -> ";
            }
            NS_LOG_INFO((*route).second << ": " << routestr);
        } 
        
        if (multiratrec < (*it).second.received) {
            NS_LOG_INFO((*it).second.received - multiratrec << ": no multi-rat used");
        }
    }
    NS_LOG_INFO("total sent: " << totalsent << ", total received: " << totalrecv << ", total reliability: " <<  ((double)totalrecv / (double)totalsent));

    std::tuple<float, uint32_t> result;
    std::get<0>(result) = (double)totalrecv / (double)totalsent;
    std::get<1>(result) = totaltime / info.size();

    return result;
}

} // namespace ns3
