import networkx as nx
from ant_optimization import AntOpt
from dataclasses import dataclass, field
from typing import List
import matplotlib.pyplot as plt
import sys
import argparse

#Class for reading a status file for multi-RAT nodes and writing route files.
@dataclass
class FileReader:
    filename: str
    file: List[List[str]] = field(default_factory=list)

    def __post_init__(self) -> None:
        with open(self.filename, 'r') as f:
            for line in f:
                lineinfo = []
                info = line.rstrip().split(",")
                self.file.append(info)

    #Construct a network based on the given file
    def ConstructNetwork(self) -> nx.MultiDiGraph:
        MDG = nx.MultiDiGraph()
        edges = {}
        for line in self.file:
            edges[(line[0], line[1], line[8])] = line[2:]

        for item in edges:  
            if not MDG.has_node(item[0]):
                MDG.add_node(item[0])
            if not MDG.has_node(item[1]):
                MDG.add_node(item[1])

            #Attempt to swap source and destination, if not possible we don't have enough information to reliably form a route
            try: 
                MDG.add_edge(item[1], item[0], ip = edges[(item[1], item[0], item[2])][0], lastseen = float(edges[item][1]), jumps = int(edges[item][2]), time = int(edges[item][3]), reliability = float(edges[item][4]), datarate = int(edges[item][5][8:-4]));
            except Exception as e:
                pass
        return MDG

    #Write the generated routes into the route file
    def WriteRoutes(self, filename: str, file: List[List[str]]) -> None:
        with open(filename, "w") as f:
            for route in file:
                out = []
                for item in route:
                    out.append(item[0] + ":" + item[1])
                f.write(",".join(out) + "\n")


if __name__ == "__main__":

    #Get the required arguments
    parser=argparse.ArgumentParser()
    parser.add_argument("infile", help = "File name for the input")
    parser.add_argument("outfile", help = "File name for the output")
    args = parser.parse_args()

    #Construct the network
    f = FileReader(args.infile)
    graph = f.ConstructNetwork()
    
    #Run ant colony optimization to determine routes
    aco = AntOpt(graph)
    graphnodes = sorted(graph.nodes)
    routes = []
    for dst in graphnodes:
        paths = aco.find_path(str(dst))
        for n, path in paths.items():
            index = int(n)
            while len(routes) <= index:
                routes.append([])
            routes[index].append((dst, path))

    #Write the routes
    f.WriteRoutes(args.outfile, routes)