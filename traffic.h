/*
 * Traffic header file
 *
 * author: Thato Semoko
 */

#ifndef TRAFFIC_H
#define TRAFFIC_H

#include <stdlib.h> 
#include <string>
#include <memory>
#include <cmath>
#include <random>
#include <fstream>
#include <time.h>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

class Load : public Application
{
    private:
        bool     running;
        uint32_t num_packets;
        uint32_t packet_size;
        uint32_t packets_sent;

        DataRate        data_rate;
        EventId         send_event;
        Address         dest_addr;
        Ptr<Socket>     src_socket;
        
        void StartApplication(void)
        {
            this->running      = true;
            this->packets_sent = 0;
            this->src_socket->Bind();
            this->src_socket->Connect(this->dest_addr);     // connect src to dest
            this->send_packet();
        }

        void StopApplication(void)
        {
            this->running      = false;
            // cancel any running events
            if(this->send_event.IsRunning()) { Simulator::Cancel(this->send_event); }

            if(this->src_socket) { this->src_socket->Close();} // close the socket
        }

        void send_packet (void)
        {
          Ptr<Packet> packet = Create<Packet> (this->packet_size);
          this->src_socket->Send (packet);

          if (++this->packets_sent < this->num_packets)
          {
            // schedule a transmission
           this->schedule_tx();
          }
        }

        void schedule_tx (void)
        {
            if (this->running)
            {
                Time t_next (Seconds (this->packet_size * 8 / static_cast<double> (this->data_rate.GetBitRate ())));
                this->send_event = Simulator::Schedule (t_next, &Load::send_packet, this);
            }
        }

    public:
        // default constructor
        Load() : running(false), num_packets(0), packet_size(0), packets_sent(0),
                 data_rate(0), send_event(), dest_addr(), src_socket(0)
        {}

        // default destructor
        ~Load() { this->src_socket = 0; }

        void setup(Ptr<Socket> socket, Address addr, uint32_t p_size, uint32_t num_p, DataRate d_rate)
        {
            this->num_packets = num_p;
            this->packet_size = p_size;
            this->data_rate   = d_rate;
            this->dest_addr   = addr;
            this->src_socket  = socket;
        }
        
};
#endif
