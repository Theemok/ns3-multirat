
#include <iostream>

#include "lib/multi-rat-utility.h"
#include "lib/packet-counter.h"
#include "lib/multi-rat-fileprotocol-helper.h"

#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-interface.h"


using namespace ns3;

int main (int argc, char *argv[])
{

  Packet::EnablePrinting();

  //Pick which components we want to monitor, usually only the packetcounter
  //LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
  //LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  //LogComponentEnable("MultiRatClient", LOG_LEVEL_INFO);
  LogComponentEnable("PacketCounter", LOG_INFO);

  //Network files
  std::string networkexec = "python multiratpy/network_gen.py";
  std::string networkfile = "multiratpy/data/staticnetwork/networkinfo";
  std::string nodefile = "multiratpy/data/staticnetwork/nodeinfo";
  std::string mratfile = "multiratpy/data/staticnetwork/mratinfo"; 

  std::string routeexec = "python multiratpy/route_gen.py";
  std::string statusfile = "multiratpy/data/staticnetwork/status";
  std::string routesfile = "multiratpy/data/staticnetwork/routes";
  
  //Import network from generated files
  std::vector<NodeContainer> containers = MultiRatUtility::ImportNetwork(networkfile, nodefile, mratfile);
  Ptr<MultiRatFileProtocolHelper> protocolhelper = CreateObjectWithAttributes<MultiRatFileProtocolHelper>(
        "UpdateRate", TimeValue(Seconds(0)), 
        "FirstUpdate", TimeValue(Seconds(100)));
  MultiRatUtility::InstallFileProtocols(protocolhelper, containers[0]);
  protocolhelper->SetFiles(routeexec, statusfile, routesfile);

  //Create udp client/server pair to generate traffic
  ApplicationContainer apps = MultiRatUtility::CreateUdpClientPair(containers[1].Get(0), containers[1].Get(containers[1].GetN() - 1), 9, 0, Seconds(1), 1024);
  apps.Start(Seconds(120));
    apps.Stop(Seconds(1120)); 

  //Connect udp pair to a packet counter
  Ptr<PacketCounter> pc = Create<PacketCounter>();
  apps.Get(0)->TraceConnect("Rx", "0",MakeCallback(&PacketCounter::HandleRX, pc));
  apps.Get(1)->TraceConnect("Tx", "0",MakeCallback(&PacketCounter::HandleTX, pc));

  //Enable the animation interface and assign colors + sizes to all nodes
  AnimationInterface anim("multi-rat-example.xml");
  MultiRatUtility::AutoAssignColor(&anim, containers);
  MultiRatUtility::AutoAssignSize(&anim, containers);

  //Run simulation
  Simulator::Stop(Seconds(1130));
  Simulator::Run();
  pc->GetResult();
  Simulator::Destroy();
}
