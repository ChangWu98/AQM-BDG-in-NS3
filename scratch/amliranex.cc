/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <string>
#include <unistd.h>
#include<stdio.h>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/amliran-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/amlirantrace.h"
#include "ns3/amliran.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("AMLIRANEX");
// set simulation time
static const double appStartTime=0.1;
static const double simDuration= 10.0;//appStop=simDuration-0.1
double appStop=simDuration-0.1;

// static void InstallAmliran(
//                          Ptr<Node> sender,
//                          Ptr<Node> receiver,
//                          uint16_t port,
//                          double startTime,
//                          double stopTime
//                          ) {
//     Ptr<AmliranSender> sendApp=CreateObject<AmliranSender>();
// 	Ptr<AmliranReceiver> recvApp=CreateObject<AmliranReceiver>();
//     Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
//     Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
//     sendApp->InitialSetup(receiverIp,port);
// 	sendApp->SetStream(numStream);
//     //sendApp->SetSourceFilePath((char*)"/home/ubuntu-ns3/ns-allinone-3.35/ns-3.35/traces/crf25_cut1.txt");
//     sender->AddApplication (sendApp);
//     receiver->AddApplication (recvApp);
//     recvApp->Bind(port);

// 	// sendApp->SetRateCallback(MakeCallback(&MyTracer::RateTrace,tracer));
// 	// recvApp->SetDelayCallback(MakeCallback(&MyTracer::DelayTrace,tracer));

//     sendApp->SetStartTime (Seconds (startTime));
//     sendApp->SetStopTime (Seconds (stopTime));
//     recvApp->SetStartTime (Seconds (startTime));
//     recvApp->SetStopTime (Seconds (stopTime));

// }

int main(int argc, char *argv[])
{
    //LogComponentEnable("TcpTest", LOG_LEVEL_ALL);

    std::string folder_name("default");
    CommandLine cmd;
    cmd.AddValue ("folder", "folder name to collect data", folder_name);
    cmd.Parse (argc, argv);
    uint16_t amlPort = 5000;

    Time::SetResolution (Time::NS);

    //MyTracer* amliranTracer;
    // std::string trace_folder;
    // {
    //     char buf[FILENAME_MAX];
    //     std::string trace_path=std::string (getcwd(buf, FILENAME_MAX))+"/traces/";
    //     trace_folder=trace_path+folder_name+"/";
    //     myTracer->MakePath(trace_folder);
    //     //TcpBbrDebug::SetTraceFolder(trace_folder.c_str());
    //     myTracer->SetTraceFolder(trace_folder.c_str());
    // }
    uint64_t linkBw=1e9;
    uint64_t link_owd=TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay=TOPO_MAX_QDELAY;

    NodeContainer nodes;
    nodes.Create (2);

    InternetStackHelper stackH;
    stackH.Install (nodes);
    
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (linkBw)));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue(MAX_PACKET_SIZE));
    p2ph.SetChannelAttribute ("Delay", TimeValue(MilliSeconds(link_owd)));
    auto bufSizeP = std::max<uint32_t> (1, linkBw * msQDelay /(8000*1500));
    p2ph.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (std::to_string(bufSizeP)+"p"));

    NetDeviceContainer devices = p2ph.Install(nodes);
    //NetDeviceContainer devices = p2ph.Install (nodes);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("10.37.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces = ipv4h.Assign (devices);

    //InstallAmliran(nodes.Get(0), nodes.Get(1), amlPort, appStartTime, appStop);

    ///****************************//
    Ptr<AmliranSender> sendApp=CreateObject<AmliranSender>();
	Ptr<AmliranReceiver> recvApp=CreateObject<AmliranReceiver>();

    nodes.Get(0)->AddApplication (sendApp);
    nodes.Get(1)->AddApplication (recvApp);

    Ipv4Address receiverIp = interfaces.GetAddress (1);
    sendApp->InitialSetup(receiverIp,amlPort);
    recvApp->Bind(amlPort);

    sendApp->SetStartTime (Seconds (appStartTime));
    sendApp->SetStopTime (Seconds (appStop));
    recvApp->SetStartTime (Seconds (appStartTime));
    recvApp->SetStopTime (Seconds (appStop));
    ///////*********************************////

    Simulator::Stop (Seconds(simDuration));
    Simulator::Run ();
    Simulator::Destroy();
	return 0;
}