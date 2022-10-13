/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <string>
#include <unistd.h>
#include<stdio.h>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-client-module.h"
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("TcpTest");
static const double startTime=0.1;
static const double simDuration= 50.5;
#define DEFAULT_PACKET_SIZE 1500
std::string qdiscTypeId = "ns3::FifoQueueDisc";//"ns3::CoDelQueueDisc";//"ns3::RedQueueDisc";//PieQueueDisc
uint32_t link_bw=(uint32_t)30e6;
uint32_t link_owd=6;
uint32_t q_delay=20;


class ChangeBw
{
public:
	ChangeBw(Ptr<NetDevice> netdevice)
	{
        m_linkRateFp= fopen(linkRateFile.c_str(),"r");
        NS_ASSERT_MSG(m_linkRateFp,"Tb file open failed");
	    fseek(m_linkRateFp,0L,SEEK_SET);

	    m_netdevice=netdevice;
	}
	//ChangeBw(){}
	~ChangeBw(){
    fclose(m_linkRateFp);
  }
	void Start()
	{
    PointToPointNetDevice *device=static_cast<PointToPointNetDevice*>(PeekPointer(m_netdevice));
    if(fscanf(m_linkRateFp,"%lf %f",&linkTime,&linkRate)) {
        device->SetDataRate(DataRate((uint64_t)(linkRate*1e6)));

        // auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, linkRate * 1e6 * q_delay / 8000);
        // int packets=bufSize/(DEFAULT_PACKET_SIZE*1);
        // Ptr<Queue<Packet> > queueA = device->GetQueue();
        // QueueSize queueSL = queueA->GetCurrentSize();
        // QueueSize queueSN = QueueSize (std::to_string(packets)+"p");
        // while(queueSL > queueSN){
        //     queueA->Dequeue();
        //     queueSL = queueA->GetCurrentSize();
        // }
        // queueA->SetMaxSize(QueueSize (std::to_string(packets)+"p")); 
        

    }
		Time next=MicroSeconds(m_gap);
		m_timer=Simulator::Schedule(next,&ChangeBw::ChangeRate,this);
	}
	void ChangeRate()
	{
		if(m_timer.IsExpired())
		{
		    PointToPointNetDevice *device=static_cast<PointToPointNetDevice*>(PeekPointer(m_netdevice));
		    if(fscanf(m_linkRateFp,"%lf %f",&linkTime,&linkRate)) {
                m_sumLinkCapacity += (int)(linkRate * 1e6) * m_gap*1e-6 / 8;
                device->SetDataRate(DataRate((uint64_t)(linkRate*1e6)));////////////Waiting to finish

                // auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, linkRate * 1e6 * q_delay / 8000);
                // int packets=bufSize/(DEFAULT_PACKET_SIZE*1);
                // Ptr<Queue<Packet> > queueA = device->GetQueue();
                // QueueSize queueSL = queueA->GetCurrentSize();
                // QueueSize queueSN = QueueSize (std::to_string(packets)+"p");
                // while(queueSL > queueSN){
                //     queueA->Dequeue();
                //     queueSL = queueA->GetCurrentSize();
                // }
                // queueA->SetMaxSize(QueueSize (std::to_string(packets)+"p")); 
            }
            int64_t now=Simulator::Now().GetMilliSeconds();
            if(now>=9800&&now<10100){
                //cout<<"now_ms: "<<now<<" sumLinkCapacity: "<<m_sumLinkCapacity<<endl;
                if(now>=9990){fseek(m_linkRateFp,0L,SEEK_SET);}
            }
		    Time next=MicroSeconds(m_gap);
		    m_timer=Simulator::Schedule(next,&ChangeBw::ChangeRate,this);
		}

	}
private:
    double linkTime = 0;//s
    float linkRate = 0;//Mbps
    uint64_t m_gap{500};//us
	Ptr<NetDevice>m_netdevice;
	EventId m_timer;
    FILE *m_linkRateFp=nullptr;
    string linkRateFile="/home/ubuntu-ns3/ns-allinone-3.35/ns-3.35/VideoSource/schedRate.txt";
    uint64_t m_sumLinkCapacity=0;//byte
    NodeContainer m_topo;
};


static NodeContainer BuildExampleTopo (uint32_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay,
                                       bool enable_random_loss=true)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    int packets=bufSize/(DEFAULT_PACKET_SIZE*1);
    NS_LOG_INFO("buffer packet "<<packets);
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (std::to_string(packets)+"p"));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    TrafficControlHelper pTCHelper;
    uint16_t handle = pTCHelper.SetRootQueueDisc (qdiscTypeId, "MaxSize", StringValue (std::to_string(packets)+"p"));
    pTCHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue (std::to_string(packets)+"p"));
    pTCHelper.Install(devices);
    Ipv4AddressHelper address;
    std::string nodeip="10.1.1.0";
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);
    if(enable_random_loss){
        std::string errorModelType = "ns3::RateErrorModel";
        ObjectFactory factory;
        factory.SetTypeId (errorModelType);
        Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
        devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }
    return nodes;
}
// ./waf --run "scratch/tcp-test --cc=bbr --folder=bbr"
int main(int argc, char *argv[])
{
    //LogComponentEnable("TcpTest", LOG_LEVEL_ALL);
    //LogComponentEnable("TcpClient", LOG_LEVEL_ALL);
    //LogComponentEnable("TcpBbr", LOG_LEVEL_ALL);
    std::string cc("bbr2");
    std::string folder_name("default");
    CommandLine cmd;
    cmd.AddValue ("cc", "congestion algorithm",cc);
    cmd.AddValue ("folder", "folder name to collect data", folder_name);
    cmd.Parse (argc, argv);
    uint32_t kMaxmiumSegmentSize=1400;
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(200*kMaxmiumSegmentSize));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(200*kMaxmiumSegmentSize));
    Config::SetDefault("ns3::TcpSocket::SegmentSize",UintegerValue(kMaxmiumSegmentSize));
    if(0==cc.compare("reno")||0==cc.compare("bic")||0==cc.compare("cubic")||
      0==cc.compare("bbr")||0==cc.compare("copa")||0==cc.compare("vegas")){}
    else{
        NS_ASSERT_MSG(0,"please input correct cc");
    }
    std::string trace_folder;
    
    {
        char buf[FILENAME_MAX];
        std::string trace_path=std::string (getcwd(buf, FILENAME_MAX))+"/traces/";
        trace_folder=trace_path+folder_name+"/";
        MakePath(trace_folder);
        //TcpBbrDebug::SetTraceFolder(trace_folder.c_str());
        TcpTracer::SetTraceFolder(trace_folder.c_str());
    }

    NodeContainer topo;
    topo=BuildExampleTopo(link_bw,link_owd,q_delay);
    Ptr<Node> h1=topo.Get(0);
    Ptr<Node> h2=topo.Get(1);

    //for utility
    TcpTracer::SetExperimentInfo(1,link_bw);
    //for loss rate
    TcpTracer::SetLossRateFlag(true);

    uint16_t serv_port = 5000;
    //install server on h2
    Address tcp_sink_addr;
    {
        Ptr<Ipv4> ipv4 = h2->GetObject<Ipv4> ();
        Ipv4Address serv_ip= ipv4->GetAddress (1, 0).GetLocal();
        InetSocketAddress socket_addr=InetSocketAddress{serv_ip,serv_port};
        tcp_sink_addr=socket_addr;
        Ptr<TcpServer> server=CreateObject<TcpServer>(tcp_sink_addr);
        h2->AddApplication(server);
        server->SetStartTime (Seconds (0.0));
    }

    uint64_t totalTxBytes = 40000*15000;
    {
        Ptr<TcpClient>  client= CreateObject<TcpClient> (totalTxBytes,TcpClient::E_TRACE_RTT|TcpClient::E_TRACE_INFLIGHT|TcpClient::E_TRACE_RATE);
        h1->AddApplication(client);
        client->ConfigurePeer(tcp_sink_addr);
        client->SetCongestionAlgo(cc);
        client->SetStartTime (Seconds (startTime));
        client->SetStopTime (Seconds (simDuration));
    }

    Ptr<NetDevice> netDevice=h1->GetDevice(0);
	ChangeBw change(netDevice);
	change.Start();

    Simulator::Stop (Seconds (simDuration+60.0));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}