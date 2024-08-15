
#include <iostream>

#include "lib/multi-rat-utility.h"
#include "lib/packet-counter.h"

#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-interface.h"

using namespace ns3;

/**
 * @brief Pick two different nodes from a container of nodes based on given seed
 * @param nodes container of nodes
 * @param multiratseed seed used to pick nodes
 * @return tuple of picked node pair indexes
 */
std::tuple<uint32_t, uint32_t>
PickNodePair(NodeContainer nodes, uint32_t multiratseed) 
{

    //Set a unique multiratseed for every experiment
    Ptr<UniformRandomVariable> ran = CreateObject<UniformRandomVariable>();
    ran->SetStream(multiratseed);

    //Select random client and server pair
    std::tuple<uint32_t, uint32_t> node;
    while (true) {
        std::get<0>(node) = ran->GetInteger(0, nodes.GetN() - 1);
        std::get<1>(node) = ran->GetInteger(0, nodes.GetN() - 1);

        if (std::get<0>(node) != std::get<1>(node)) break;
    }
    return node;
}

/**
 * @brief Find node in container based on it's location
 * @param nodes container of nodes
 * @param x location of node
 * @param y location of node
 * @return index of found node, 0 if none found
 */
uint32_t
FindNodeBylocation(NodeContainer nodes, uint32_t x, uint32_t y) 
{
    //Loop through nodes and check their x and y location
    for (uint32_t node = 0; node < nodes.GetN(); node++) {
        Ptr<MobilityModel> mob = nodes.Get(node)->GetObject<MobilityModel>();
        if (mob->GetPosition().x == x && mob->GetPosition().y == y) {
            return node;
        }
    }
    return 0;
}

/**
 * @brief Simulates the given containers of nodes with clients in the specified spots
 * @param runtime duration of the simulation
 * @param containers list of node containers
 * @param clientlocations list of locations of udp client/server pairs
 * @param experimentseed seed for given experiment
 * @return udp pair reliability and average delivery time in microseconds
 */
std::tuple<float, uint32_t>
Simulate(Time runtime, std::vector<NodeContainer> containers, std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> clientlocations, uint32_t experimentseed) {

    //Set a unique multiratseed for every experiment
    Ptr<UniformRandomVariable> ran = CreateObject<UniformRandomVariable>();
    ran->SetStream(experimentseed);

    //Register pair to the packet counter
    Ptr<PacketCounter> pc = Create<PacketCounter>();

    NS_LOG_UNCOND("Adding pairs:");
    for (uint32_t location = 0; location < clientlocations.size(); location++) {
        //Pick nodes by given location list
        uint32_t node1 = FindNodeBylocation(containers[1], std::get<0>(clientlocations[location]), std::get<1>(clientlocations[location]));
        uint32_t node2 = FindNodeBylocation(containers[1], std::get<2>(clientlocations[location]), std::get<3>(clientlocations[location]));
        NS_LOG_UNCOND(node1 <<" -> " << node2 << " (" << std::get<0>(clientlocations[location]) << "," << std::get<1>(clientlocations[location]) << ") (" << std::get<2>(clientlocations[location]) << "," << std::get<3>(clientlocations[location]) << ")" );
        
        //Add udp client to the picked nodes
        ApplicationContainer apps = MultiRatUtility::CreateUdpClientPair(containers[1].Get(node1), containers[1].Get(node2), location + 1, 0, Seconds(ran->GetValue(0.9,1.1)), 1024);
        apps.Get(0)->TraceConnect("Rx", std::to_string(location), MakeCallback(&PacketCounter::HandleRX, pc));
        apps.Get(1)->TraceConnect("Tx", std::to_string(location), MakeCallback(&PacketCounter::HandleTX, pc));
        
        //Avoid starting the applications at exactly the same time
        apps.Start(Seconds(110.5 + ran->GetValue(0,5)));
        apps.Stop(runtime - Seconds(5));
    }

    //Enable the animation interface and assign colors + sizes to all nodes
    std::string animname = "no-multi-rat-experiment.xml";
    if (containers.size() > 1) {
        animname = "multi-rat-experiment.xml";
    }
    AnimationInterface anim(animname);
    MultiRatUtility::AutoAssignColor(&anim, containers);
    MultiRatUtility::AutoAssignSize(&anim, containers);

    //Run simulation
    Simulator::Stop (runtime);
    Simulator::Run();
    std::tuple<float, uint32_t> result = pc->GetResult();
    Simulator::Destroy();

    return result;
}

/**
 * @brief Set up a network according to given parameters
 * @param protocolhelper filehelper protocol
 * @param seed seed for the creation of the networks
 * @param multiratseed amount of different seeds to simulate for the placement of multi-rat clients
 * @param networkcount amount of networks to add to the simulation
 * @param datarate datarate of networks after first
 * @param range datarate of networks after first
 * @return vector of nodecontainers sorted by the network they belong to, starting with the multi-rat nodes
 */
std::vector<NodeContainer>
SetupNetwork(Ptr<MultiRatFileProtocolHelper> protocolhelper, uint32_t seed, uint32_t multiratseed, uint32_t networkcount, std::string datarate, std::string range) {

    //Network files
    std::string networkexec = "python multiratpy/network_gen.py";
    std::string networkfile = "multiratpy/data/experiment/networkinfo";
    std::string nodefile = "multiratpy/data/experiment/nodeinfo";
    std::string mratfile = "multiratpy/data/experiment/mratinfo";

    //Route files
    std::string routeexec = "python multiratpy/route_gen.py";
    std::string statusfile = "multiratpy/data/experiment/status";
    std::string routesfile = "multiratpy/data/experiment/routes";

    //Generate network through python and import + register it
    system((networkexec + " " + networkfile + " " + nodefile + " " + mratfile + " " + std::to_string(seed) + " " + std::to_string(multiratseed) + " " + std::to_string(networkcount)).c_str());

    //Set custom values in network file as the network generator will fill this in randomly
    //First network has a default setting as it is the main network
    std::ofstream network;
    network.open(networkfile);
    network << "0,OfdmRate9Mbps,60\n";
    for (uint32_t nid = 1; nid < networkcount; nid++) {
        network << std::to_string(nid) << "," << datarate << "," << range << "\n";
    }
    network << std::flush;
    network.close();

    //Import network
    std::vector<NodeContainer> containers = MultiRatUtility::ImportNetwork(networkfile, nodefile, mratfile);
    MultiRatUtility::InstallFileProtocols(protocolhelper, containers[0]);

    //configure protocol helper
    protocolhelper->SetFiles(routeexec, statusfile, routesfile);

    return containers;
}

std::tuple<float, uint32_t, float, uint32_t> 
experiment(uint32_t numclients, uint32_t seed, uint32_t multiratseed, uint32_t udpseed, uint32_t networkcount, std::string datarate, std::string range, Time duration,bool simnomultirat)
{
    NS_LOG_UNCOND("multiratseed: " << multiratseed << ", clients: " << udpseed << ", network count: " << networkcount << ", datarate: " << datarate << ", range: " << range);

    //seed for this experiment
    uint32_t experimentseed = multiratseed * 2000 + udpseed;

    //Generate multi-rat network
    Ptr<MultiRatFileProtocolHelper> protocolhelper = CreateObjectWithAttributes<MultiRatFileProtocolHelper>(
        "UpdateRate", TimeValue(Seconds(0)), 
        "FirstUpdate", TimeValue(Seconds(100)));

    std::vector<NodeContainer> containers = SetupNetwork(protocolhelper, seed, multiratseed, networkcount, datarate, range);

    //Generate given amount of udp clients and save their location
    //Necessary because node ids are not the same between a normal network and a multi-rat network
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> clientlocations;
    std::vector<uint32_t> nodesused;
    uint32_t clientsadded = 0;
    uint32_t attempts = 0;
    while (clientsadded < numclients) {

        //Pick two random nodes
        std::tuple<uint32_t, uint32_t> clients = PickNodePair(containers[1], experimentseed + attempts++);

        //Check if these nodes have been used in a udp client pair already
        if (std::find(nodesused.begin(), nodesused.end(), std::get<0>(clients)) == nodesused.end() 
        && std::find(nodesused.begin(), nodesused.end(), std::get<1>(clients)) == nodesused.end() ) {

            //Add nodes to used nodes list
            nodesused.push_back(std::get<0>(clients));
            nodesused.push_back(std::get<1>(clients));

            //Register location of both nodes
            std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> clientlocation;
            Ptr<MobilityModel> mob1 = containers[1].Get(std::get<0>(clients))->GetObject<MobilityModel>();
            std::get<0>(clientlocation) = mob1->GetPosition().x;
            std::get<1>(clientlocation) = mob1->GetPosition().y;

            Ptr<MobilityModel> mob2 = containers[1].Get(std::get<1>(clients))->GetObject<MobilityModel>();
            std::get<2>(clientlocation) = mob2->GetPosition().x;
            std::get<3>(clientlocation) = mob2->GetPosition().y;

            //Add locations to list of client server pair locations
            clientlocations.push_back(clientlocation);
            clientsadded++;
        }
    }

    //Simulate multi-rat network
    std::tuple<float, uint32_t, float, uint32_t> result;
    std::tuple<float, uint32_t> res = Simulate(duration, containers, clientlocations, experimentseed);
    std::get<0>(result) = std::get<0>(res);
    std::get<1>(result) = std::get<1>(res);

    //Check if we want to simulate the normal version of this network
    std::tuple<float, uint32_t> result2;
    if (simnomultirat) {

        //Simulate normal network
        Ptr<MultiRatFileProtocolHelper> protocolhelper = CreateObjectWithAttributes<MultiRatFileProtocolHelper>(
            "UpdateRate", TimeValue(Seconds(0)), 
            "FirstUpdate", TimeValue(Seconds(100)));
        containers = SetupNetwork(protocolhelper, seed, multiratseed, 1, datarate, range);
        res = Simulate(duration, containers, clientlocations, experimentseed);
        std::get<2>(result) = std::get<0>(res);
        std::get<3>(result) = std::get<1>(res);
    }

    return result;
}

/**
 * @brief Run experiments where there are more udp clients added every round of simulations
 * @param networkseed seed for the creation of the networks
 * @param maxclients maximum amount of udp clients to add to simulation
 * @param mratseeds amount of different seeds to simulate for the placement of multi-rat clients
 * @param clientseeds amount of different seeds to simulate for the placement of udp clients
 * @param networkcount amount of networks to add to the simulation
 * @param duration duration of simulations
 * @param file file to store results in
 */
void
RunCongestionExperiments(uint32_t networkseed, uint32_t maxclients, uint32_t mratseeds, uint32_t clientseeds, uint32_t networkcount, Time duration, std::string file)
{
    //Loop through every number of udp clients we want to simulate
    for(uint32_t clientnum = 1; clientnum <= maxclients; clientnum++) {

        //Open files
        std::ofstream reliabilityfilenormal;
        std::ofstream timefilenormal;
        std::ofstream reliabilityfile;
        std::ofstream timefile;
        reliabilityfilenormal.open(file + "-" + std::to_string(clientnum) + "-normal-reliability.csv");
        timefilenormal.open(file + "-" + std::to_string(clientnum) + "-normal-time.csv");
        reliabilityfile.open(file + "-" + std::to_string(clientnum) + "-reliability.csv");
        timefile.open(file + "-" + std::to_string(clientnum) + "-time.csv");

        //Amount of seeds to try for positions of multi-rat nodes
        for (uint32_t mratseed = 0; mratseed < mratseeds; mratseed++) {    
            std::string seperator;

              //Amount of seeds to try for positions of udp clients
            for (uint32_t clientseed = 1; clientseed <= clientseeds; clientseed++) {
                //Run simulation
                std::tuple<float, uint32_t, float, uint32_t>  result = experiment(clientnum, networkseed, mratseed, clientseed, networkcount, "OfdmRate9Mbps", "60", duration, true);
                
                //Write results to files
                reliabilityfile << seperator << std::get<0>(result) << std::flush;
                timefile << seperator << std::get<1>(result) << std::flush;
                reliabilityfilenormal << seperator << std::get<2>(result) << std::flush;
                timefilenormal << seperator << std::get<3>(result) << std::flush;
                seperator = ",";
            }

            //End line in files
            reliabilityfile << "\n" << std::flush;
            timefile << "\n" << std::flush;
            reliabilityfilenormal << "\n" << std::flush;
            timefilenormal << "\n" << std::flush;
        }

        //Close files
        reliabilityfile.close();
        timefile.close();
        reliabilityfilenormal.close();
        timefilenormal.close();
    }
}


/**
 * @brief Run experiments where the second and onwards network added to the simulation has different datarates and ranges than the first
 * @param seed seed for the creation of the networks
 * @param mratseeds amount of different seeds to simulate for the placement of multi-rat clients
 * @param clientseeds amount of different seed to simulate for the placement of udp clients
 * @param networkcount amount of networks to add to the simulation
 * @param duration duration of simulations
 * @param file file to store results in
 */
void
RunRangeRateExperiments(uint32_t networkseed, uint32_t mratseeds, uint32_t clientseeds, uint32_t networkcount, Time duration, std::string file)
{

    std::vector<std::string> dataranges;
    std::vector<std::string> ranges;

    //Rates to evaluate
    dataranges.insert(dataranges.end(), {"OfdmRate9Mbps", "OfdmRate12Mbps", "OfdmRate18Mbps", "OfdmRate24Mbps"});
    //Ranges to evaluate
    ranges.insert(ranges.end(), {"60", "110", "160"});

    //Open the normal case files
    std::ofstream reliabilityfilenormal;
    std::ofstream timefilenormal;
    reliabilityfilenormal.open(file + "normal-reliability.csv");
    timefilenormal.open(file + "normal-time.csv");

    //Perform simulation for all datarate and range combinations
    //First loop we also include normal case without multi-rat as to match the exact location of udp clients
    bool first = true;
    for(auto datarangeit = dataranges.begin(); datarangeit != dataranges.end(); datarangeit++) {
        for(auto rangeit = ranges.begin(); rangeit != ranges.end(); rangeit++) {

            //Open the multi-rat case files
            std::ofstream reliabilityfile;
            std::ofstream timefile;
            reliabilityfile.open(file + "-" + (*datarangeit) + "-" + (*rangeit) + "-reliability.csv");
            timefile.open(file + "-" + (*datarangeit) + "-" + (*rangeit) + "-time.csv");

            //Amount of seeds to try for positions of multi-rat nodes
            for (uint32_t mratseed = 10; mratseed < mratseeds + 10; mratseed++) {    
                std::string seperator;

                //Amount of seeds to try for positions of udp clients
                for (uint32_t clientseed = 1; clientseed <= clientseeds; clientseed++) {
                    //Run simulation
                    std::tuple<float, uint32_t, float, uint32_t>  result = experiment(1, networkseed, mratseed, clientseed, networkcount, (*datarangeit), (*rangeit), Seconds(1000), first);
                    
                    //Write results to files
                    reliabilityfile << seperator << std::get<0>(result) << std::flush;
                    timefile << seperator << std::get<1>(result) << std::flush;
                    if (first) {
                        reliabilityfilenormal << seperator << std::get<2>(result) << std::flush;
                        timefilenormal << seperator << std::get<3>(result) << std::flush;
                    }
                    seperator = ",";
                }

                //End line in files
                reliabilityfile << "\n" << std::flush;
                timefile << "\n" << std::flush;
                if (first) {
                    reliabilityfilenormal << "\n" << std::flush;
                    timefilenormal << "\n" << std::flush;
                }
            }

            //Close files
            reliabilityfile.close();
            timefile.close();
            if (first) {
                first = false;
                reliabilityfilenormal.close();
                timefilenormal.close();
            }
        }
    }
}

int 
main (int argc, char *argv[])
{
    Packet::EnablePrinting();
    PacketMetadata::Enable();

    //Pick which components we want to monitor, usually only the packetcounter
    //LogComponentEnable("UdpServer", LOG_INFO);
    //LogComponentEnable("UdpClient", LOG_INFO);
    //LogComponentEnable("MultiRatClient", LOG_INFO);
    LogComponentEnable("PacketCounter", LOG_INFO);

    //Datarate and range experiment example
    //RunRangeRateExperiments(13, 2, 2, 2, Seconds(1000), "multiratpy/results/rangerate");

    //Congestion experiment example
    //RunCongestionExperiments(20773, 10, 9, 10, 2, Seconds(1000), "multiratpy/results/congestion");

    //You can also run one singular experiment
    experiment(5, 13, 10, 4, 2, "OfdmRate9Mbps", "60", Seconds(1000), true);
}
