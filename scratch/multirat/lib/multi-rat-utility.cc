
#include "multi-rat-utility.h"

#include "multi-rat-client.h"
#include "multi-rat-fileprotocol-helper.h"

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/yans-wifi-helper.h"


namespace ns3 {

void
MultiRatUtility::SetNodePosition(Ptr<Node> node, int32_t x, int32_t y) 
{
    //Create a mobility helper and install it to the specified node.
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX",
                                    DoubleValue(x),
                                    "MinY",
                                    DoubleValue(y),
                                    "DeltaX",
                                    DoubleValue(0),
                                    "DeltaY",
                                    DoubleValue(0),
                                    "GridWidth",
                                    UintegerValue(1),
                                    "LayoutType",
                                    StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(node);
}

void
MultiRatUtility::AutoAssignColor(AnimationInterface *anim, std::vector<NodeContainer> nodelists) 
{
    //Create random number generator with set seed to keep colors consistent
    Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>();
    random->SetStream(2);

    //Loop over nodecontainers and assign a seperate color to each
    for (std::vector<NodeContainer>::iterator nodes = nodelists.begin(); nodes != nodelists.end(); nodes++) {
        int8_t red = random->GetInteger(0,255);
        int8_t blue = random->GetInteger(0,255);
        int8_t green = random->GetInteger(0,255);
        for (NodeContainer::Iterator node = (*nodes).Begin(); node != (*nodes).End(); node++) {
            (*anim).UpdateNodeColor(*node, red, blue, green);
        }
    }
}

void
MultiRatUtility::AutoAssignSize(AnimationInterface *anim, std::vector<NodeContainer> nodelists) 
{
    uint32_t nodelistssize = nodelists.size();

    //Loop over nodecontainers and assign a seperate size to each
    for (uint32_t nodes = 0; nodes < nodelistssize; nodes++) {
        uint32_t size = 40 - (((nodes > 0 ? nodes : 1) - 1) * 40 / nodelistssize); 
        for (NodeContainer::Iterator node = nodelists[nodes].Begin(); node != nodelists[nodes].End(); node++) {
             (*anim).UpdateNodeSize (*node, size, size);
        }
    }
}

std::vector<NodeContainer>
MultiRatUtility::ImportNetwork(std::string networkfile, std::string nodefile, std::string multiratfile) 
{
    std::vector<NodeContainer> containers;

    //Configure multi-rat nodes
    std::vector<MultiRatInfo> mrnodes = ReadMultiRatInfo(multiratfile);
    NodeContainer multiratnodes = ConfigureMultiRatNodes(mrnodes);
    containers.push_back(multiratnodes);

    //Create normal nodes by channel
    std::map<uint32_t, NetworkInfo> networks = ReadNetworkInfo(networkfile);
    std::map<uint32_t, std::vector<NodeInfo>> nodes = ReadNodeInfo(nodefile);
    for (std::map<uint32_t, std::vector<NodeInfo>>::iterator channel = nodes.begin(); channel != nodes.end() ; channel++) {

        //Configure nodes
        NodeContainer nodes = ConfigureNodes(channel->second);
        containers.push_back(nodes);

        //Add multi-rat nodes that belong to this channel
        NodeContainer compatiblenodes;
        //iterate over every multi-rat node
        for (uint32_t mrnum = 0; mrnum < mrnodes.size(); mrnum++) {
            //Iterate over channels multi-rat node is compatible with and add if compatible
            std::vector<uint32_t> channellist = mrnodes[mrnum].channels;
            if (std::find(channellist.begin(), channellist.end(), channel->first) != channellist.end()) {
                compatiblenodes.Add(multiratnodes.Get(mrnum));
            }
        }

        //Add both nodecontainers together
        compatiblenodes.Add(nodes);
        ConfigureNetwork(compatiblenodes, "10." + std::to_string(channel->first) + ".0.0", networks[channel->first]);

    }

    return containers;
}

std::vector<MultiRatInfo> 
MultiRatUtility::ReadMultiRatInfo(std::string nodefile) 
{
  std::vector<MultiRatInfo> info;

  std::ifstream file;
  file.open(nodefile);

  std::string line;
  std::string segment;
  std::string item;

  while (std::getline (file, line)) {
    MultiRatInfo node;
    std::stringstream linestream(line);

    //Loop over channels that the node is in.
    std::getline(linestream, segment, ',');
    std::stringstream segmentstream(segment);
    while(std::getline(segmentstream, item, ':')) {
      node.channels.push_back(std::stoul(item, nullptr, 0));
    }

    //x coordinate
    std::getline(linestream, segment, ',');
    node.x = std::stoul(segment, nullptr, 0);

    //y coordinate
    std::getline(linestream, segment, ',');
    node.y = std::stoul(segment, nullptr, 0);
    
    info.push_back(node);
  }
  return info;
}

std::map<uint32_t, std::vector<NodeInfo>> 
MultiRatUtility::ReadNodeInfo(std::string nodefile) 
{
  std::map<uint32_t, std::vector<NodeInfo>>  info;

  std::ifstream file;
  file.open(nodefile);

  std::string line;
  std::string segment;

  while (std::getline (file, line)) {
    NodeInfo node;
    std::stringstream linestream(line);

    //channel
    std::getline(linestream, segment, ',');
    uint32_t channel = std::stoul(segment, nullptr, 0);

    //x coordinate
    std::getline(linestream, segment, ',');
    node.x = std::stoul(segment, nullptr, 0);

    //y coordinate
    std::getline(linestream, segment, ',');
    node.y = std::stoul(segment, nullptr, 0);
    
    info[channel].push_back(node);
  }
  return info;
}

std::map<uint32_t, NetworkInfo>
MultiRatUtility::ReadNetworkInfo(std::string networkfile) 
{
  std::map<uint32_t, NetworkInfo> info;

  std::ifstream file;
  file.open(networkfile);

  std::string line;
  std::string segment;

  while (std::getline (file, line)) {
    NetworkInfo network;
    std::stringstream linestream(line);

    //channel
    std::getline(linestream, segment, ',');
    uint32_t channel = std::stoi(segment);

    //bitrate
    std::getline(linestream, segment, ',');
    network.bitrate = segment;

    //range
    std::getline(linestream, segment, ',');
    network.range = std::stoi(segment);

    info[channel] = network;
  }
  return info;
}

NodeContainer 
MultiRatUtility::ConfigureMultiRatNodes(std::vector<MultiRatInfo> mrnodeinfo) 
{
    uint32_t nodecount = mrnodeinfo.size();

    //Create multi rat nodes
    NodeContainer multiratnodes;
    multiratnodes.Create(nodecount);

    //Install internet stack
    InternetStackHelper internetStack;
    internetStack.Install(multiratnodes);

    //Loop over every newly created node
    for (uint32_t node = 0; node < nodecount; node++) {

        //Install a MultiRatClient on every node
        Ptr<MultiRatClient> mrc = Create<MultiRatClient>();
        multiratnodes.Get(node)->AddApplication(mrc);

        //Configure the node according to given node info
        MultiRatUtility::SetNodePosition(multiratnodes.Get(node), mrnodeinfo[node].x, mrnodeinfo[node].y);
    }
    return multiratnodes;
}

NodeContainer
MultiRatUtility::ConfigureNodes(std::vector<NodeInfo> nodeinfo) 
{
    uint32_t nodecount = nodeinfo.size();

    NodeContainer nodes;
    nodes.Create(nodecount);

    //Install internet stack on nodes
    InternetStackHelper internetStack;
    internetStack.Install(nodes);

        //Loop over every newly created node
    for (uint32_t node = 0; node < nodecount; node++) {

        //Configure the node according to given node info
        MultiRatUtility::SetNodePosition(nodes.Get(node), nodeinfo[node].x, nodeinfo[node].y);
    }

    return nodes;
}

void
MultiRatUtility::ConfigureNetwork(NodeContainer nodes, std::string baseAddress, NetworkInfo networkinfo) 
{

    //Configure wifiphy
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper();
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    //For testing purposes and the mesh implementations disdain for grid layouts we avoid the default propagation loss model
    //Instead we use a static range which also simplifies changing the bandwidth of connections.
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(networkinfo.range));
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.EnablePcapAll(std::string("mp"));
    
    //Configure mesh
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller("ns3::Dot11sStack", "Root", Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")));
    mesh.SetSpreadInterfaceChannels(MeshHelper::ZERO_CHANNEL);
    mesh.SetMacType("RandomStart", TimeValue(Seconds(0.1)));
    mesh.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue (networkinfo.bitrate));

    //Install mesh + wifiphy
    NetDeviceContainer meshDevices = mesh.Install(wifiPhy, nodes);

    //Assign addresses
    Ipv4AddressHelper address;
    address.SetBase(Ipv4Address(baseAddress.c_str()), "255.255.0.0");
    Ipv4InterfaceContainer interfaces = address.Assign(meshDevices);

}

void
MultiRatUtility::InstallFileProtocols(Ptr<MultiRatFileProtocolHelper> manager, NodeContainer nodes) 
{
    for(NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
        for (uint32_t application = 0; application < (*node)->GetNApplications(); application++) {
            Ptr<MultiRatClient> client = DynamicCast<MultiRatClient>((*node)->GetApplication(application));
            if (client) {
                Ptr<MultiRatFileProtocol> protocol = Create<MultiRatFileProtocol>();
                client->SetRoutingProtocol(protocol);
                manager->RegisterProtocol(protocol);
            }
        }
    }
}

ApplicationContainer
MultiRatUtility::CreateUdpClientPair(Ptr<Node> node1, Ptr<Node> node2, uint16_t port, uint32_t maxpackets, Time interval, uint16_t size) 
{
    //Echo server
    UdpServerHelper server(port);
    ApplicationContainer apps = server.Install(node2);

    //Get address of echo server
    Ptr<Ipv4L3Protocol> ipv4 = node2->GetObject<Ipv4L3Protocol>();
    Ipv4Address dest = ipv4->GetInterface(1)->GetAddress(0).GetLocal();

    //Echo client
    UdpClientHelper client(dest, port);
    client.SetAttribute("MaxPackets", UintegerValue(maxpackets));
    client.SetAttribute("Interval", TimeValue(interval));
    client.SetAttribute("PacketSize", UintegerValue(size));
    apps.Add(client.Install(node1));
    
    return apps;
}

} // namespace ns3