
#include <iostream>

#include "lib/multi-rat-utility.h"
#include "lib/packet-counter.h"

#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-interface.h"

using namespace ns3;

void
experiment()
{
    //duration of simulation
    uint32_t runtime = 1200;

    //Network files
    std::string networkexec = "python multiratpy/network_gen.py";
    std::string networkfile = "multiratpy/data/dynamicroutes/networkinfo";
    std::string nodefile = "multiratpy/data/dynamicroutes/nodeinfo";
    std::string mratfile = "multiratpy/data/dynamicroutes/mratinfo";

    //Route files
    std::string routeexec = "python multiratpy/route_gen.py";
    std::string statusfile = "multiratpy/data/dynamicroutes/status";
    std::string routesfile = "multiratpy/data/dynamicroutes/routes";

    //Import the handmade network
    std::vector<NodeContainer> containers = MultiRatUtility::ImportNetwork(networkfile, nodefile, mratfile);

    //Create protocolhelper
    Ptr<MultiRatFileProtocolHelper> protocolhelper = Create<MultiRatFileProtocolHelper>();

    //If you want the protocol to never update its routes you can set custom attributes for it as such
    //Ptr<MultiRatFileProtocolHelper> protocolhelper = CreateObjectWithAttributes<MultiRatFileProtocolHelper>(
    //"UpdateRate", TimeValue(Seconds(0)), 
    //"FirstUpdate", TimeValue(Seconds(100)));

    //Install protocols
    MultiRatUtility::InstallFileProtocols(protocolhelper, containers[0]);
    protocolhelper->SetFiles(routeexec, statusfile, routesfile);

    //Register udp pair to the packet counter
    Ptr<PacketCounter> pc = Create<PacketCounter>();
    ApplicationContainer apps = MultiRatUtility::CreateUdpClientPair(containers[1].Get(0), containers[1].Get(3), 1, 0, Seconds(1), 1024);
    apps.Get(0)->TraceConnect("Rx", std::to_string(0), MakeCallback(&PacketCounter::HandleRX, pc));
    apps.Get(1)->TraceConnect("Tx", std::to_string(0), MakeCallback(&PacketCounter::HandleTX, pc));
    apps.Start(Seconds(120));   
    apps.Stop(Seconds(120 + 1000)); 

    //Add udp pair to cause interference
    ApplicationContainer interferenceapp = MultiRatUtility::CreateUdpClientPair(containers[3].Get(0), containers[3].Get(6), 2, 0, Seconds(0.01), 1024);
    interferenceapp.Start(Seconds(120 + 250));
    interferenceapp.Stop(Seconds(120 + 500));

    //Enable the animation interface and assign colors + sizes to all nodes
    AnimationInterface anim("multi-rat.xml");
    MultiRatUtility::AutoAssignColor(&anim, containers);
    MultiRatUtility::AutoAssignSize(&anim, containers);

    //Run simulation
    Simulator::Stop (Seconds(runtime));
    Simulator::Run();
    pc->GetResult();
    Simulator::Destroy();
}

int 
main (int argc, char *argv[])
{
    PacketMetadata::Enable();

    //Pick which components we want to monitor, usually only the packetcounter
    //LogComponentEnable("UdpServer", LOG_INFO);
    //LogComponentEnable("UdpClient", LOG_INFO);
    //LogComponentEnable("MultiRatClient", LOG_INFO);
    LogComponentEnable("PacketCounter", LOG_INFO);

    //Run experiment
    experiment();
}
