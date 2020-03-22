#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import os

directory = "./test_pcaps/"
out_file  = "./test_pcaps/data_new.csv"
o_file    = "./test_pcaps/load_test-10-0"
p_file    = "./test_pcaps/live_servers.txt"

time_data =  []         # time series for the data
load_data = []          # load data
live_servers =  []
energy =  []
w_energy =  []
time_series = {}

def create_load_graphs():
    load =0
# run through the original file for time values
    for filename in os.listdir(directory):

        if filename.endswith(".csv"):
            print(filename)
            f_input = directory+filename

            with open(f_input, "r") as f:
                # read the first line 
                f.readline()

                for line in f.readlines():
                    d = line.split(",")
                    if len(d) > 1:
                        t = int(float(d[0]))

                        if t in time_series: 
                            time_series[t] += int(d[1])
                        else: 
                            time_series[t] = int(d[1])
"""
# plot graphs
# -- load graph --
    plt.plot(time_series.keys(), time_series.values(), 'r')
    plt.ylabel("Requests per second")
    plt.xlabel("Time (s)")
    plt.title("")
    plt.savefig("../report/Figures/requests.png")
    plt.clf()

    capacity = 0.75*20000*32

    for value in time_series.values(): 
        load_data.append(value/float(capacity))
        
    plt.plot(time_series.keys(), load_data, 'b')
    plt.ylabel("Load")
    plt.xlabel("Time (s)")
    plt.title("")
    plt.savefig("../report/Figures/load_t.png")
    plt.clf()
"""
create_load_graphs()

def export_csv():
    with open(out_file, "w") as f:

        for value in time_series.values():
            f.write(str(value)+"\n")
        f.close()

def get_active_servers():
    active_servers = []
    with open(p_file, 'r') as g:

        for line in g.readlines():
            live_servers.append(int(line))
            active_servers.append(32)
    g.close()


    plt.plot(time_series.keys(), live_servers[:34], 'b', label="with load balancer")
    plt.plot(time_series.keys(), active_servers[:34], 'r', label="without load balancer")
    plt.ylabel("live servers")
    plt.xlabel("Time (s)")
    plt.title("")
    plt.legend(loc=2)
    plt.savefig("../report/Figures/live_servers.png")
    plt.clf()

def plot_transitions(filename):
    transitions = []
    b_k = []
    with open(filename, 'r') as g:

        for line in g.readlines():
            transitions.append(int(line))
            b_k.append(10)
    g.close()

    plt.plot(time_series.keys(), transitions[:34], 'b', label="server transitions")
    plt.plot(time_series.keys(), b_k[:34], 'r', label="allowed server transitions")
    plt.ylabel("server transitions")
    plt.xlabel("Time (s)")
    plt.title("")
    plt.legend(loc=1)
    plt.savefig("../report/Figures/transitions.png")
    plt.clf()

plot_transitions("test_pcaps/server_transitions.txt")
"""
# divide the load by the capacity and plot it

# -- energy graph: column chart --
for load,server in zip(load_data, live_servers):
    power = (63 + (92 - 63)*load)*server
    energy.append(power) 

for load in (load_data):
    w_power = (63 + (92 - 63)*load)*17
    w_energy.append(w_power)

plt.plot(time_data, energy, 'b', label="with ELB")
plt.plot(time_data, w_energy, 'r', label="without ELB")
plt.ylabel("Power consumption")
plt.xlabel("Time (s)")
plt.title("")
plt.legend(loc=2)
plt.savefig("./test_pcaps/energy_opt.png")
plt.clf()
"""
