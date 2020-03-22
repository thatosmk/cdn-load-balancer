#include "network.h"
#include "traffic.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <cstring>   // Needed for strings
#include <fstream>  // Needed for file stream objects
#include <sstream>
#include <time.h>

#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-flow-probe.h"

using namespace std;
using namespace ns3;

void export_data(string filename, vector<int> live_servers)
{
    // 
    ofstream out(filename.c_str(), ios_base::app);
    for(int server: live_servers)
    {
        out << server << endl;
    }
    out.close();
}

vector<double> load_data(string filename)
{
    // define server capacity
    int capacity = 0.75*20000*32;

    // create a input file stream object
    ifstream in_file(filename.c_str());
    string line;
    string time, data, error;

    vector<double> load_d;

    // check if the file exists
    if(!in_file) { cerr << "File could not be opened!" << endl; }

    while(getline(in_file, line, ','))
    {
        istringstream iss(line); 
        while(!iss.eof())
        {
            iss >>  data; 
            // add to vector
            load_d.push_back(stod(data)/capacity);
        }
    }
   return load_d;
}

int main(int argc, char *argv[])
{

    Time::SetResolution(Time::NS);

    int num_servers = 8;
    int num_clusters = 22;

    // create the network with a load threshold of 0.75
    cout << "setting up network..."<< endl;
    Network cdn(num_servers, num_clusters, 0.75);

    LoadBalancer lb;
    /*
    vector<Server> servers = cdn.get_server_nodes();

    cout << "creating traffic..."<< endl;
    for(Server s: servers)
    {
        for(unsigned int i=0; i<s.getNodes().GetN();i++ )
        {
            uint32_t sender   = i*2 % num_servers;

            uint16_t sinkPort = 8080;
            // string dest = "172.16.11.3";
            Address sinkAddress (InetSocketAddress (s.getIPContainer().GetAddress(i), sinkPort));
            PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
            ApplicationContainer sinkApps = packetSinkHelper.Install (s.getNodes().Get(i));
            sinkApps.Start (Seconds (0.));
            sinkApps.Stop (Seconds (3600.));

            // configure senders
            Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (s.getNodes().Get (sender), TcpSocketFactory::GetTypeId ());
            //ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

            // need to make different datarates
            int rate = rand() % 100 +1;
            uint32_t packets  = (rand() % 5000) + 400;
            uint32_t p_size   = (rand() % 1000) + 400;
            // simulate for an hour
            uint32_t s_time   = (rand() % 1000) + 1;
            uint32_t e_time   = (rand() % 3600) + 1800;

            Ptr<Load> app = CreateObject<Load> ();
            app->setup (ns3TcpSocket, sinkAddress, p_size, packets, DataRate (to_string(rate)+"Mbps"));
            // install the load balancer on the incoming sockets
            s.getNodes().Get (sender)->AddApplication (app);
            app->SetStartTime (Seconds (s_time));
            app->SetStopTime (Seconds (e_time));

            CsmaHelper csma;
            csma.EnablePcap("../../../../Documents/CDN/load_test", s.getNetDev().Get(0), true);
        }
    }

    */
    // install a load balancer and
    // grab the load
    cout << "running offline load balancing algorithm..."<< endl;
    vector<int> l_servers = lb.offline_lb(cdn, load_data("./test_pcaps/data_new.csv"));
    vector<int> transitions = lb.offline_lb2(cdn, load_data("./test_pcaps/data_new.csv"), 100);
    export_data("./test_pcaps/live_servers.txt", l_servers);
    export_data("./test_pcaps/server_transitions.txt", transitions);

    //cout << "exported data"<< endl;

    /*
    Simulator::Stop (Seconds (3600));
    Simulator::Run ();
    
	Simulator::Destroy ();
    */

    return 0;
}
