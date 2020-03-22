/*
 * network header file
 *
 * author: Thato Semoko
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h> 
#include <string>
#include <memory>
#include <cmath>
#include <random>
#include <fstream>
#include <time.h>
#include <iostream>

#include "traffic.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/simulator.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

class Server
{
    private:
        // private members of the class
        NodeContainer nodes;
        NetDeviceContainer net_nodes;
        Ipv4InterfaceContainer ip_interface;

        int uuid, cluster_id, size;
        double load_amt;
        const double capacity = 0.7;
        bool on, spare;

    public:
        // default constructor
        Server();
        Server(NodeContainer *tor, int port, int uuid, int cluster_id, int size) : uuid(uuid), cluster_id(cluster_id), size(size), on(false), spare(false)
        {
            // add the TOR switch to the rack
            this->nodes.Add(tor->Get(port));
            // create servers in the rack
            this->nodes.Create(size);

            // setup access connection
            CsmaHelper csma;
            csma.SetChannelAttribute("DataRate", StringValue("40Gbps"));
            csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));

            // install the csma on the nodes
            this->net_nodes = csma.Install(this->nodes);

            // install the IP stack
            InternetStackHelper ip_stack;

            for(int i=1; i<=this->size; i++)
            {
                ip_stack.Install(this->nodes.Get(i));
            }

        }
        ~Server() {}

        NetDeviceContainer getNetDev() { return this->net_nodes; }
        NodeContainer getNodes() { return this->nodes; }

        int getID();
        double getLoad() { return this->load_amt; }

        double getCapacity(void) { return this->capacity; }

        int getClusterID();

        Ipv4InterfaceContainer getIPContainer(void) { return this->ip_interface; }

        void setIP(string base, string mask)
        {
            Ipv4AddressHelper address;

            // address the rack
            address.SetBase(base.c_str(), mask.c_str());
            this->ip_interface = address.Assign(this->net_nodes);
            //cout << "Server range in a switch IP: "<< base << endl;
        }
};

class Cluster
{
    private:
        int uuid, num_servers, num_sws;
        double load, load_threshold;
        vector<Server> servers;
        NodeContainer aggr_tor;
        NetDeviceContainer aggr_tor_net;
        Ipv4InterfaceContainer ip_interface;
        string rack_ip, rack_mask;

    public:
        // default constructor
        Cluster();

        Cluster(NodeContainer *origin, vector<string> ips, int s, int cluster_id, double load_thresh) : uuid(cluster_id), num_servers(s), load_threshold(load_thresh)
        {
            this->aggr_tor.Add(origin->Get(0)); 

            // how many TOR switches do we need
            this->num_sws = int(sqrt(this->num_servers));

            this->aggr_tor.Create(this->num_sws);

            // access connections on the switches
            CsmaHelper csma;
            csma.SetChannelAttribute("DataRate", StringValue("40Gbps"));
            csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));

            this-> aggr_tor_net = csma.Install(this->aggr_tor);

            for(signed int i=1; i<=this->num_sws; i++)
            {
                // install an IP stack on the switches
                InternetStackHelper ip_stack;
                Ipv4AddressHelper address;

                ip_stack.Install(this->aggr_tor.Get(i));
                // IP address assign
                address.SetBase(ips.at(i-1).c_str(), "255.255.255.0");
                this->ip_interface = address.Assign(this->aggr_tor_net.Get(i));
            }

        }

        void createServers(int port)
        {
            // install the IP stack
            InternetStackHelper ip_stack;
            Ipv4AddressHelper address;


            // address the tor switches
            // create servers for this rack
            Server rack(&this->aggr_tor,port,0,this->uuid, this->num_servers);
            rack.setIP(this->rack_ip, this->rack_mask);


            servers.push_back(rack);

        }

        ~Cluster() {}


        int getID();

        double getLoad(void)
        {
            double total_load = 0;

            for(Server s: servers) 
            {
                total_load += s.getLoad();
            }

            return total_load;
        }
        vector<Server> getServers() { return this->servers; }
        NodeContainer getTOR(void) { return this->aggr_tor;  }

        void setRackIP(string b, string mask) { this->rack_ip = b; this->rack_mask = mask; }
};

class Network
{
    private:
        int num_servers, num_clusters;
        double load, load_threshold;
        vector<Cluster> clusters;
        NodeContainer origin;
        vector<NetDeviceContainer> origin_nets;
        PointToPointHelper p2p;

    public:
        // default constructor
        Network() {}

        Network(int s, int c, double t) : num_servers(s), num_clusters(c), load_threshold(t)
        {
            this->origin.Create(this->num_clusters+1);


            this->p2p.SetDeviceAttribute("DataRate", StringValue("40Gbps"));
            this->p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));
            InternetStackHelper ip_stack;
            ip_stack.Install(this->origin);


            int sw_id = 1;
            int rack_id = 1;
            // address the clusters (aggregate layer)
            for(int i=0; i< this->num_clusters; i++)
            {
                vector<string> ips_bases;

                for(int j=1; j<= pow(this->num_servers, 0.5); j++)
                {
                    string b = "10.";
                    string id = to_string(sw_id++)+".1.0";
                    string ip   = b+id;
                    ips_bases.push_back(ip);
                }

                Cluster cluster(&this->origin, ips_bases, this->num_servers,i, 0.75);


                for(int j=1; j<= pow(this->num_servers, 0.5); j++)
                {
                    string b = "172.16.1";
                    string id = to_string(rack_id++);
                    string ip   = b+id+".0";
                    
                    // set rack IP for this cluster
                    cluster.setRackIP(ip, "255.255.255.0");
                    cluster.createServers(j);
                }

                this->clusters.push_back(cluster);
            }

            // connect the aggregation to the core
            this->connect_aggrs();

        }

        ~Network() {}

        // custom copy contructor
        /*
        Network(const Network &other) : num_servers(other.num_servers), num_clusters(other.num_clusters)
        {
            // init the data block	
            this->data = unique_ptr<unsigned char[]>(new unsigned char[other.height*other.width]);

        }
        */
        void connect_aggrs()
        {
        
            /*
             * handle the p2p connection between origin and aggr (aggr is inside agg_tor)
             */


            // access connections
            CsmaHelper csma;
            csma.SetChannelAttribute("DataRate", StringValue("40Gbps"));
            csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));


            for(unsigned int i=0; i<this->clusters.size(); i++)
            {
                NetDeviceContainer aggr_dev;
                aggr_dev = this->p2p.Install(this->origin.Get(0), this->clusters.at(i).getTOR().Get(1));
                this->origin_nets.push_back(aggr_dev);

            }

            NetDeviceContainer aggr_dev;
            aggr_dev = csma.Install(this->origin);

            for(unsigned int i=0; i<this->origin_nets.size(); i++)
            {
                Ipv4AddressHelper address;
                string b = "192.168.";
                string network = to_string(i+1);
                string o = ".0";
                string base   = b+network+o;

                address.SetBase(base.c_str(),"255.255.255.0");
                Ipv4InterfaceContainer interface_p2p1 = address.Assign(this->origin_nets.at(i));
            }


        }

        vector<Server> get_server_nodes() 
        {
            vector<Server> servers;

            for(int i=0; i<this->num_clusters; i++) 
            {
                for(Server server: clusters.at(i).getServers())
                {
                    servers.push_back(server); 
                }
            }

            return servers;
        }
        

        vector<int> opt_loadbalancer(vector<double> load_sequence)
        {
            // initialise m_t servers that need to be live to
            // serve the load
            vector<int> servers;
            
            // find out how many servers, m_t, can serve load_t
            for(double load: load_sequence)
            {
                int m_t = (int)((load/this->load_threshold)*this->num_servers);

                if (m_t < this->num_servers) { servers.push_back(m_t); }
                else { servers.push_back(this->num_servers); }
            }

            return servers;
        }

        
        PointToPointHelper getP2P() { return this->p2p; }

        double get_threshold() { return this->load_threshold; }
        int get_servers() { return this->num_servers; }
        int get_clusters(){ return this->num_clusters; }
        vector<Cluster> getClusters() { return this->clusters; }
        vector<NetDeviceContainer> getNetDevs() { return this->origin_nets; }

};

class LoadBalancer  : public Application
{
    private:
        uint32_t total_packets;
        vector<uint32_t> load_per_time;
        clock_t start_time;
        clock_t stop_time;


    public:
        LoadBalancer() : total_packets(0), load_per_time(), stop_time()
        {
            start_time = clock();
            cout << "Installed Load Balancer" << endl;
        }

        ~LoadBalancer() {}

        uint32_t get_total_packets() { return this->total_packets;}

        uint32_t timestamp(void)
        {
            return (clock() - this->start_time)*1.0/CLOCKS_PER_SEC; 
        }

        void online_lb(Network &cdn, vector<double> traffic, int kappa, int tau)
        {
            /* some servers need to be turned off */
            
            /* check if current live servers can manage current load */
            int lambda_t = (int)((load/cdn.get_threshold())*cdn.get_servers());
            int m_t = cdn.get_servers();
            
            if(lambda_t > m_t )
            {
                /* insufficient live servers for current load */ 
                // drop lambda_t - m_t load
                // server the rest of the load, 
                // turn on (lambda_t - m_t)/threshold servers and assign as spare in t+1 
                cout << "insufficient live servers for load" << endl;
            }
            else
            {
                /* live servers numbered (lambda_t/threshold) +1 ... m_t are spare */ 
                // -------- spare capacity rule
                int spare = int(kappa*cdn.get_servers());
                int c_spares = m_t - lambda_t; // current spares

                if (c_spares < spare) { cout << "spare rule ends" << endl; }
                // -------- hibernate rule
            }
        }

        vector<int> offline_lb2(Network &cdn, vector<double> traffic, int k) 
        {
            vector<int> servers;
            vector<int> transitions;
            int i = 0; 
            // find out how many servers, m_t, can serve load_t
            for(double load: traffic)
            {
                int m_t = (int)((load/cdn.get_threshold())*cdn.get_servers());
                
                if (m_t < 1) { servers.push_back(1);}   // minimum set of servers

                else if (m_t < cdn.get_servers()) 
                { 
                    // check server transitions that need to take place
                    // by checking m_(t-1)
                    if(i>0)
                    {
                        int transition = abs(m_t - servers[i]);
                        transitions.push_back(transition);
                        cout << "Transitions: "<< transition << endl;
                    }
                    servers.push_back(m_t); 
                    i+=1;
                }
                else { servers.push_back(cdn.get_servers()); } // all servers be firing
            }

            return transitions;
        }
        vector<int> offline_lb(Network &cdn, vector<double> traffic) 
        {
            vector<int> servers;

            // find out how many servers, m_t, can serve load_t
            for(double load: traffic)
            {
                int m_t = (int)((load/cdn.get_threshold())*cdn.get_servers());
                
                if (m_t < 1) { servers.push_back(1);}   // minimum set of servers

                else if (m_t < cdn.get_servers()) 
                { 
                    servers.push_back(m_t); 
                }
                else { servers.push_back(cdn.get_servers()); } // all servers be firing
            }

            return servers;
        }

        /* 
         * schedule an event to expire after delay
        void real_time_lb(bool running, const Time &delay, Load::Load mem_ptr, Load *object)
        {
            if(running)
            {
                this->send_event = Simulator::Schedule(delay, &mem_ptr, object);
                cout << Simulator::Now ().GetSeconds () << endl;
            }
        }
         */
};
#endif
