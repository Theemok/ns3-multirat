import networkx as nx
from typing import List, Set, Union, Dict
from dataclasses import dataclass, field
import random


#Manages the graph and updates pheromone values
@dataclass
class GraphObj:
    graph: nx.MultiDiGraph
    evaporation_rate: float

    #Set the pheromone value for this edge
    def set_pheromone(self, src: str, dst: str, index: int, pher: float) -> None:
        self.graph[src][dst][index]["pheromone"] = pher

    #Get the pheromone value for this edge
    def get_pheromone(self, src: str, dst: str, index: int) -> float:
        return self.graph[src][dst][index]["pheromone"]

    #Lower all pheromone based on the evaporation_rate value
    def apply_evaporation(self) -> None:
        for e in self.graph.edges:
            self.graph[e[0]][e[1]][e[2]]["pheromone"] = self.graph[e[0]][e[1]][e[2]]["pheromone"] * (1 - self.evaporation_rate)

    #Add pheromone to this edge
    def put_pheromone(self, src: str, dst: str, index: int, pher) -> None:
        self.graph[src][dst][index]["pheromone"] += pher

    #Get the cost of traversing this edge
    def get_cost(self, src: str, dst: str, index: int) -> float:
        info = self.graph[src][dst][index]
        reliability = info["reliability"] if info["reliability"] > 0.01 else 0.01
        return (info["jumps"] / info["datarate"]) / (reliability**2) + 0.1

    #Get edge options for ant to travel to
    def get_connection_options(self, src: str, unvisited_nodes: List[str]) -> List[tuple[str, int]]:
        return [x[1:] for x in self.graph.edges if x[0] == src and x[1] in unvisited_nodes]

    #Get the neighbours of this node
    def get_neighbours(self, node: str) -> List[str]:
        return list(self.graph.neighbors(node))

    #Set the next hop of this node
    def set_next_hop(self, node: str, nexthop: Union[tuple[str, int],None]) -> None:
        self.graph.nodes[node]["next_hop"] = nexthop

    #Check to ensure a next hop exists on this node
    def has_next_hop(self, node: str) -> bool:
        return self.graph.nodes[node]["next_hop"] != None

    #Get the next hop of this node
    def get_next_hop(self, node: str) -> Union[tuple[str, int],None]:
        return self.graph.nodes[node]["next_hop"]

    #Get list of nodes that are directly or indirectly accessible from this node
    def get_connected_nodes(self, node:str) -> List[str]:
        known = [node]
        explore = [node]
        while len(explore) > 0:
            edges = self.graph.in_edges(explore[0])
            for e in edges:
                if not e[0] in known:
                    known.append(e[0])
                    explore.append(e[0])
            explore = explore[1:]
        return known

#Manages the ants
@dataclass
class Ant:
    graph: GraphObj
    src: str
    dst: str
    alpha: float = 0.0
    beta: float = 0.0
    visited_nodes: Set = field(default_factory=set)
    path: List[str] = field(default_factory=list)
    is_fit: bool = False
    solution: bool = False
    path_cost: float = 0.0

    def __post_init__(self) -> None:
        self.cur_node = self.src

    #Check if ant is at its final destination
    def reached_destination(self) -> bool:
        return self.cur_node == self.dst

    #Get a list of unvisited neighbours
    def get_unvisited_neighbours(self) -> List[str]:
        return [x for x in self.graph.get_neighbours(self.cur_node) if x not in self.visited_nodes]

    #Set the next node to visit
    def set_next_node(self, next_hop: tuple[str, int]) -> None:
        self.path.append(next_hop)
        self.cur_node = next_hop[0]

    #Get the desirability of an edge
    def get_desirability(self, pher: float, cost: float) -> float:
        return pow(pher, self.alpha) * pow((1 / cost), self.beta)

    #Get the desirability of all edges
    def get_all_desirability(self, options: list[tuple[str, int]]) -> float:
        total = 0.0
        for n in options:
            pher = self.graph.get_pheromone(self.cur_node, n[0], n[1])
            cost = self.graph.get_cost(self.cur_node, n[0], n[1])
            total += self.get_desirability(pher, cost)
        return total

    #Calculate the probabilities of taking every edge
    def calc_probs(self, options: list[tuple[str, int]]) -> list[tuple[str, int, float]]:
        probs: list[str, int, float] = []
        all_desirability = self.get_all_desirability(options)
        for n in options:
            pher = self.graph.get_pheromone(self.cur_node, n[0], n[1])
            cost = self.graph.get_cost(self.cur_node, n[0], n[1])
            node_desirability = self.get_desirability(pher, cost)
            probs.append((n[0], n[1], node_desirability / all_desirability))
        return probs

    #Pick a random edge to choose based on the probabilities
    def pick_random(self, probs: list[str, int, float]) -> tuple[str, int]:
        sorted_probabilities = sorted(probs, key = lambda x: -x[2])
        pick = random.random()
        current = 0.0
        for node, connection, fitness in sorted_probabilities:
            current += fitness
            if current > pick:
                return (node, connection)

    #Choose the next edge to take
    def choose_next(self) -> Union[str, None]:
        unvisited_neighbors = self.get_unvisited_neighbours()
        if len(unvisited_neighbors) == 0:
            return None
        options = self.graph.get_connection_options(self.cur_node, unvisited_neighbors)
        if self.solution:
            solution = max(options, key=lambda x: self.graph.get_pheromone(self.cur_node, x[0], x[1]))
            self.graph.set_next_hop(self.cur_node, solution)
            return solution
        probs = self.calc_probs(options)
        return self.pick_random(probs)

    #Take a step
    def take_step(self) -> bool:
        self.visited_nodes.add(self.cur_node)
        next_node = self.choose_next()
        if not next_node:
            return False
        self.path_cost += self.graph.get_cost(self.cur_node, next_node[0], next_node[1])
        self.set_next_node(next_node)
        return True

    #Deposit pheromone on the edges visited by the ant
    def put_pheromone_on_path(self) ->None:
        for n in range(len(self.path)):
            src = self.path[n - 1][0]
            if n == 0:
                src = self.src
            dst = self.path[n][0]
            new_pheromone = 1 / self.path_cost
            self.graph.put_pheromone(src, dst, self.path[n][1], new_pheromone)


@dataclass
class AntOpt:
    graph: nx.MultiDiGraph
    max_steps: int = 32
    evaporation_rate: float = 0.001
    alpha: float = 0.7
    beta: float = 0.3
    iterations: int = 100

    #Reset the graph so it can be used again
    def init_graph(self):
        self.graph_obj = GraphObj(self.graph, self.evaporation_rate)
        for e in self.graph.edges:
            self.graph_obj.set_pheromone(e[0], e[1], e[2], 1.0)
        for n in self.graph_obj.graph.nodes:
            self.graph_obj.set_next_hop(n, None)

    #Perform a search for one ant
    def forward_search(self, ant: Ant) -> bool:
        for _ in range(self.max_steps):
            if ant.reached_destination():
                return True
            if not ant.take_step():
                break
        return False

    #Perform a search with all ants
    def search_ants(self, dst: str, ant_num: int) -> None:
        for y in range(self.iterations):
            antlist = []
            for x in range(ant_num):
                src = random.choice(list(self.graph.nodes))
                ant = Ant(self.graph_obj, src, dst, alpha = self.alpha, beta = self.beta)
                if self.forward_search(ant):
                    antlist.append(ant)
            self.graph_obj.apply_evaporation()
            for ant in antlist:
                ant.put_pheromone_on_path()

    #Deploy a solution ant
    def solution_ant(self, src: str, dst: str) -> Ant:
        ant = Ant(self.graph_obj, src, dst, solution=True)
        while not ant.reached_destination():
            if self.graph_obj.has_next_hop(ant.cur_node):
                break
            if not ant.take_step():
                break
        return ant

    #Finds a path from any node in the network towards the destination node
    def find_path(self, dst: str, ant_num: int = 100) -> Union[str, None]:
        self.init_graph()
        random.seed(0)
        self.search_ants(dst, ant_num)

        for n in self.graph_obj.graph.nodes:
            self.solution_ant(n, dst)

        connected_nodes = self.graph_obj.get_connected_nodes(dst)
        out = {}

        for n in self.graph_obj.graph.nodes:
            solution = self.graph_obj.get_next_hop(n)
            if solution and n in connected_nodes:
                out[n] = self.graph[n][solution[0]][solution[1]]["ip"]

        return out
