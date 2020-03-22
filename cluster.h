/*
 * Cluster header file
 *
 * author: Thato Semoko
 */

#ifndef CLUSTER_H
#define CLUSTER_H

#include <stdlib.h> 
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

struct SERVER {
    /* 
    * servers in multiple racks form a cluster
    */
    int size;
    NodeContainer nodes;
    NetDeviceContainer nodes_network;

    SERVER(int n)
    {
        this->size = n;
    }

    void install(NodeContainer *host, int port)
    {
        this->nodes.Add(host->Get(port));
        this->nodes.Create(this->size);

        CsmaHelper csma_helper;
        csma_helper.SetChannelAttribute("DataRate", StringValue("1Mbps"));
        csma_helper.SetChannelAttribute("Delay", TimeValue(NanoSeconds(500)));

        NetDeviceContainer net_dev;
        net_dev = csma_helper.Install(this->nodes); 
        this->nodes_network = csma_helper.Install(this->nodes);

        InternetStackHelper ip_stack;

        for(int i=1; i<=this->size; i++)
        {
            ip_stack.Install(this->nodes.Get(i));
        }
    }
};

struct ORIGIN
{
    // assumption is that there is always one origin server
    int size;
    NodeContainer origin;
    NetDeviceContainer origin_network;

    ORIGIN()
    {
        this->size = 1; 
        this->origin.Create(this->size);
    }
};
namespace network
{

    void createCluster(int n, NodeContainer host);
};
#endif
