/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef AMLIRAN_H
#define AMLIRAN_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include<stdio.h>
using namespace std;
namespace ns3 {

/* ... */
const int pacSize = 100; 
const int numStream = 1;
const int rateWindow = 15000;//us,rate calculation window,30ms
const int qDelayTh = 30000;//us,RLC queue delay threshould,40ms
const uint64_t TOPO_DEFAULT_PDELAY = 1;//in ms:1ms
const uint32_t TOPO_MAX_QDELAY = 10;//in ms:10ms
const uint64_t MAX_PACKET_SIZE = 1500;
const uint32_t RTT = 2*TOPO_DEFAULT_PDELAY + qDelayTh*1e-3;//ms
const double streamPriority[numStream]={1};
const int64_t sendRateUpdateInterval = 20000;//us,30ms
const int64_t fbInfoEvalWindow = 20000;//us,30ms
const double sumStartRate = 15e6;

// char buf_t[FILENAME_MAX];
// std::string tbSched_path=std::string (getcwd(buf_t, FILENAME_MAX))+"/VideoSources/";;
// static const string tbSchedFile = tbSched_path+"UE_4_Tb_Size.txt";


}

#endif /* AMLIRAN_H */

