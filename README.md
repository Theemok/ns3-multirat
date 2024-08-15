# ns3-multirat
Application which simulates multi-RAT behaviour using 802.11s mesh networks with varying characteristics.

Created for ns-3.39

You can install it by dropping both folders into the ns-3.39 folder.

Contains multiple experiments:
example.cc - Generates a multi-RAT network according to a seed and simulates it.
static-example.cc - Imports a previously made multi-RAT network and simulates it.
dynamic-routes.cc - Imports a previously made multi-RAT network and congestion to the optimal route a certain percentage of the time during the simulation.
experiments.cc - Runs large amount of experiments with varying network characteristics or network congestion.
