import argparse
from perlin_noise import PerlinNoise
import networkx as nx
from dataclasses import dataclass, field
import random, sys, getopt

@dataclass
class NetworkGen:
    # network attributes
    network_count: int
    network_size: int
    multirat_node_count: int
    # random configuration
    network_seed: int = 0
    multi_rat_seed: int = 0
    # noise generation
    octaves: int = 7
    noise_threshold: float = 0.0

    def __post_init__(self) -> None:
        self.multi_rat_network = nx.Graph()
        self.multi_rat_network_backup = nx.Graph()
        self.protected_positions = []

    # get all neighbouring nodes for a position
    def get_neighbours(self, network: nx.Graph, x: int, y: int) -> list[str]:
        return [node for node, data in network.nodes(data=True) if data['pos'] in [(x + 1,y),(x - 1,y),(x,y + 1),(x,y - 1)]]

    # generate a single graph
    def generate_network(self) -> nx.Graph:
        # generate network based on perlin noise to create clumps of nodes
        network = nx.Graph()
        noise = PerlinNoise(octaves=self.octaves, seed=random.random())
        for x in range(self.network_size):
            for y in range(self.network_size):
                node_noise = noise([x/self.network_size, y/self.network_size])
                if node_noise < self.noise_threshold:
                    #add node to graph
                    name = str(x * self.network_size + y)
                    network.add_node(name, pos=(x,y), multirat=False)

                    #connect nodes to neighbours for later
                    neighbours = self.get_neighbours(network, x, y)
                    for node in neighbours:
                        network.add_edge(name, node)

        # cull connected components with less than x nodes to improve user graph visibility
        for component in list(nx.connected_components(network)):
            if len(component) < 5:
                for node in component:
                    network.remove_node(node)
        return network

    # generate an amount of compliant networks equal to network_count combine them into one graph
    def generate_all_networks(self) -> None:
        self.multi_rat_network = nx.Graph()
        self.protected_positions = []
        firstnetwork = nx.Graph()
        for networknum in range(self.network_count):
            network = nx.Graph()
            while True:
                network = self.generate_network()

                if len(list(nx.connected_components(network))) == 1:
                    if self.multi_rat_network.number_of_nodes() == 0:
                        if len(network.nodes) > 30:
                            firstnetwork = network
                            break
                        continue
                    matchingnodes = 0
                    for node1 in network:
                        for node2 in firstnetwork:
                            if network.nodes[node1]["pos"] == firstnetwork.nodes[node2]["pos"]:
                                matchingnodes += 1
                                break
                    if matchingnodes >= 5:
                        break
            # change node name to avoid naming conflicts
            nx.relabel_nodes(network, lambda x: str(networknum) + "-" + x, copy=False)
            # add attribute to know which network the node belongs to
            nx.set_node_attributes(network, [networknum], "type")
            # add first and last node of network to protected positions
            # these are purely preserved because the echo clients from ns3 go on these nodes
            nodelist = list(network.nodes(data=True))
            self.multi_rat_network = nx.compose(self.multi_rat_network, network)

    # check if multiratnetwork is fully connected
    def check_network_connected(self) -> bool:
        return len(list(nx.connected_components(self.multi_rat_network))) == 1

    # add one multirat node to the graph
    def add_multirat_node(self) -> bool:
        # select random position
        x = random.randrange(self.network_size)
        y = random.randrange(self.network_size)

        # check if there are multiple nodes in position to combine into a multirat node
        # this also allows us to skip checking for existing multirat nodes
        # as there is only one node in their position
        # also check against protected positions
        sameposnodes = [n for n,d in self.multi_rat_network.nodes(data=True) if d['pos'] == (x, y)]
        if len(sameposnodes) > 1 and (x, y) not in self.protected_positions:
            #collect all the network types on the existing nodes
            ratttypes = [self.multi_rat_network.nodes[node]["type"][0] for node in sameposnodes]

            # delete nodes in same position
            for node in sameposnodes:
                self.multi_rat_network.remove_node(node)

            # create multirat node
            self.multi_rat_network.add_node("multirat", pos=(x,y), type=ratttypes)

            #connect multirat node to all neighbours
            neighbours = self.get_neighbours(self.multi_rat_network, x, y)
            for node in neighbours:
                if self.multi_rat_network.nodes[node]["type"][0] in ratttypes:
                    self.multi_rat_network.add_edge("multirat", node)

            # add node to list of protected positions
            self.protected_positions.append((x, y))
            return True
        return False

    # add all multirat nodes to graph
    def add_all_multirat_nodes(self) -> bool:
        if self.network_count == 1:
            return True
        multiratnodenum = 0
        # adds multirat nodes equal to multirat_node_count and gives up after a number of attempts
        while True:

            #add a multirat node and if succesfull up the counter and rename the node or it will overwrite
            if self.add_multirat_node():
                multiratnodenum += 1
                nx.relabel_nodes(self.multi_rat_network, {"multirat" : "multirat" + "-" + str(multiratnodenum)}, copy=False)

            if multiratnodenum == self.multirat_node_count:
                return True

        return False

    # generate the final multirat network
    def generate_multirat_network(self) -> nx.Graph:
        # generate all seperate networks
        random.seed(self.network_seed)
        self.generate_all_networks()
        self.multi_rat_network_backup = self.multi_rat_network

        random.seed(self.multi_rat_seed)
        while True:
            # add multirat nodes and repeat process if failed
            # check if entire multirat network is connected
            if (self.multirat_node_count == 0) or (self.add_all_multirat_nodes() and self.check_network_connected()):
                return self.multi_rat_network
            # restore network for new attempt at adding multi-rat nodes
            self.multi_rat_network = self.multi_rat_network_backup


if __name__ == "__main__":

    #Get the required arguments
    parser=argparse.ArgumentParser()
    parser.add_argument("networkinfo", help = "File name for network output")
    parser.add_argument("nodeinfo", help = "File name for the nodes output")
    parser.add_argument("mratinfo", help = "File name for the multi-rat nodes output")
    parser.add_argument("networkseed", type = int, help = "Seed used to create the network")
    parser.add_argument("multiratseed", type = int, help = "Seed used to place multi-rat nodes in the network")
    parser.add_argument("networkcount", type = int, help = "How many networks exist. Setting this to one will disable multi-rat")
    args = parser.parse_args()

    #Generate the network
    a = NetworkGen(args.networkcount, 10, 4, network_seed=args.networkseed, multi_rat_seed=args.multiratseed)
    MG = a.generate_multirat_network()

    #Write node info
    with open(args.nodeinfo, 'w') as f:
        for node in MG.nodes:
            if len(MG.nodes[node]['type']) == 1:
                pos = MG.nodes[node]['pos']
                pos = list(map(lambda x: x * 50, pos))
                types = [str(x) for x in MG.nodes[node]['type']]
                f.write(types[0] + "," + str(pos[0]) + "," + str(pos[1]) + "\n")

    #Write multi-rat node info
    with open(args.mratinfo, 'w') as f:
        for node in MG.nodes:
            if len(MG.nodes[node]['type']) > 1:
                pos = MG.nodes[node]['pos']
                pos = list(map(lambda x: x * 50, pos))
                types = [str(x) for x in MG.nodes[node]['type']]
                f.write(":".join(types) + "," + str(pos[0]) + "," + str(pos[1]) + "\n")

    #Write network info
    with open(args.networkinfo, 'w') as f:
        #Add information to network
        for i in range(a.network_count):
            f.write(str(i) + "," + "OfdmRate9Mbps" + "," + "60" + "\n")