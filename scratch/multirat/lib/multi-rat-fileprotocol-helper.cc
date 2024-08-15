
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <windows.h>

#include <map>

#include "multi-rat-fileprotocol-helper.h"

#include "multi-rat-utility.h"
#include "multi-rat-client.h"
#include "multi-rat-fileprotocol.h"

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/yans-wifi-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiRatFileProtocolHelper");

NS_OBJECT_ENSURE_REGISTERED(MultiRatFileProtocolHelper);

TypeId
MultiRatFileProtocolHelper::GetTypeId()
{
  static TypeId tid = TypeId("ns3::MultiRatFileProtocolHelper")
                          .SetParent<Object>()
                          .SetGroupName("MultiRat")
                          .AddConstructor<MultiRatFileProtocolHelper>()
                          .AddAttribute("UpdateRate",
                                "Time between the recalculation of routes, 0 seconds for no updates after initial",
                                TimeValue(Seconds(10)),
                                MakeTimeAccessor(&MultiRatFileProtocolHelper::m_updaterate),
                                MakeTimeChecker())
                          .AddAttribute("FirstUpdate",
                                "Time before the first route update",
                                TimeValue(Seconds(10)),
                                MakeTimeAccessor(&MultiRatFileProtocolHelper::m_firstupdate),
                                MakeTimeChecker());
  return tid;
}

MultiRatFileProtocolHelper::MultiRatFileProtocolHelper():
  m_updaterate(Seconds(10)),
  m_firstupdate(Seconds(10)),
  m_execute(""),
  m_status(""),
  m_routes("")
{
  NS_LOG_FUNCTION(this);
  Simulator::Schedule(Seconds(0), &MultiRatFileProtocolHelper::Start, this);
}

void
MultiRatFileProtocolHelper::Start()
{
  NS_LOG_FUNCTION(this);
  m_updateEvent = Simulator::Schedule(m_firstupdate, &MultiRatFileProtocolHelper::Update, this);
}

MultiRatFileProtocolHelper::~MultiRatFileProtocolHelper()
{
  NS_LOG_FUNCTION(this);
  Simulator::Cancel(m_updateEvent);
  m_execute.clear();
  m_status.clear();
  m_routes.clear();
}

void
MultiRatFileProtocolHelper::SetFiles(std::string execute, std::string status, std::string routes)
{
  NS_LOG_FUNCTION(this << execute << status << routes);
  m_execute = execute;
  m_status = status;
  m_routes = routes;
}

void
MultiRatFileProtocolHelper::Update() 
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_execute != "" && m_status != "" && m_routes != "");
  
  if (m_protocols.size() > 1) {

    // Write the protocol status to the status file
    PostStatus();

    // Call external program to determine routes
    system((m_execute + " " + m_status + " " + m_routes).c_str());

    // Update protocol routes
    UpdateRoutes();

    // Update compatible nodes list for channels
    UpdateCompatibleClients();

  }
  
  //Since we will only be testing static nodes we limit ourselves to one update
  if (m_updaterate != Seconds(0)) {
      m_updateEvent = Simulator::Schedule(m_updaterate, &MultiRatFileProtocolHelper::Update, this);
  }
}

void
MultiRatFileProtocolHelper::UpdateRoutes()
{
  NS_LOG_FUNCTION(this);

  std::ifstream routefile;
  routefile.open(m_routes);

  std::string line;
  std::string segment;
  std::string item;

  for (std::vector<Ptr<MultiRatFileProtocol>>::iterator protocol = m_protocols.begin(); protocol != m_protocols.end(); protocol++) {
    if (routefile) {
      // Get line from routes file
      std::getline (routefile, line);
      std::stringstream linestream(line);
      std::map<uint32_t, Ipv4Address> routes;

      // Loop over routes in line
      while(std::getline(linestream, segment, ',')) {

        //node id
        std::stringstream segmentstream(segment);
        std::getline(segmentstream, item, ':');
        uint32_t target = std::stoul(item, nullptr, 0);

        //address
        std::pair <Ipv4Address, uint8_t> hopinfo;
        std::getline(segmentstream, item, ':');
        Ipv4Address info = Ipv4Address(item.c_str());
        
        // Add route to routes map
        routes[target] = info;
      }
      // Set the routes for this protocol
      (*protocol)->SetRoutes(routes);
    }
  }
  routefile.close();
} 

void
MultiRatFileProtocolHelper::PostStatus() 
{
  NS_LOG_FUNCTION(this);

  std::ofstream statusfile(m_status);

  // Loop over every protocol
  for (std::vector<Ptr<MultiRatFileProtocol>>::iterator client = m_protocols.begin(); client != m_protocols.end(); client++) {
    
    // Get the neighbour data and paste every entry in a line
    std::map<Ipv4Address, NeighbourData> data = (*client)->GetNeighbours();
    for(std::map<Ipv4Address, NeighbourData>::iterator ip = data.begin(); ip != data.end(); ++ip) {
      statusfile
      << (*client)->GetClient()->GetNode()->GetId() << ","
      << ip->second.node << "," 
      << ip->first << "," 
      << ip->second.lastseen.GetSeconds() << "," 
      << (uint32_t) ip->second.hops << "," 
      << ip->second.deliverytime << ","
      << ip->second.reliability << ","
      << ip->second.datarate << ","
      << ip->second.channelid << "\n" << std::flush;
    }
  }
  statusfile.close();
}

void
MultiRatFileProtocolHelper::UpdateCompatibleClients() 
{
  NS_LOG_FUNCTION(this);

  // Loop through every protocol
  std::map<uint32_t, std::vector<uint32_t>> compatabilitymap;
  for (std::vector<Ptr<MultiRatFileProtocol>>::iterator client = m_protocols.begin(); client != m_protocols.end(); client++) {
    
    // Add compatible node to corresponding channel in compatibility map
    std::vector<uint32_t> channels = (*client)->GetClient()->GetChannels();
    for (std::vector<uint32_t>::iterator channel = channels.begin(); channel != channels.end(); channel++) {
      compatabilitymap[(*channel)].push_back((*client)->GetClient()->GetNode()->GetId());
    }
  }

  // Set the compatibility on every protocol
  for (std::vector<Ptr<MultiRatFileProtocol>>::iterator client = m_protocols.begin(); client != m_protocols.end(); client++) {
    (*client)->SetCompatibleMultiRatClients(compatabilitymap);
  }
}

void
MultiRatFileProtocolHelper::RegisterProtocol(Ptr<MultiRatFileProtocol> protocol) 
{
  NS_LOG_FUNCTION(this << protocol);
  m_protocols.push_back(protocol);
}

} // namespace ns3