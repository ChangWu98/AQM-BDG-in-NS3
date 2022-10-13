/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

/**
 * \ingroup examples
 * \file cttc-3gpp-channel-a-capacity-test.cc
 * \brief Capacity Test
 *
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/grid-scenario-helper.h"
#include "ns3/log.h"
#include "ns3/antenna-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mytrace.h"
#include "ns3/throughputRW-module.h"
#include <stdarg.h>
#include <string>
#include <math.h>
#include <iostream>
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/netanim-module.h"
#include "ns3/buildings-module.h"

#include <ns3/lte-rrc-protocol-ideal.h>
#include <ns3/lte-rrc-protocol-real.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-chunk-processor.h>
#include <ns3/epc-ue-nas.h>
#include <ns3/names.h>
#include <ns3/nr-rrc-protocol-ideal.h>
#include <ns3/nr-gnb-mac.h>
#include <ns3/nr-gnb-phy.h>
#include <ns3/nr-ue-phy.h>
#include <ns3/nr-ue-mac.h>
#include <ns3/nr-gnb-net-device.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/nr-ch-access-manager.h>
#include <ns3/bandwidth-part-gnb.h>
#include <ns3/bwp-manager-gnb.h>
#include <ns3/bwp-manager-ue.h>
#include <ns3/nr-rrc-protocol-ideal.h>
#include <ns3/epc-helper.h>
#include <ns3/epc-enb-application.h>
#include <ns3/epc-x2.h>
#include <ns3/nr-phy-rx-trace.h>
#include <ns3/nr-mac-rx-trace.h>
#include <ns3/nr-bearer-stats-calculator.h>
#include <ns3/bandwidth-part-ue.h>
#include <ns3/beam-manager.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/three-gpp-propagation-loss-model.h>
#include <ns3/three-gpp-spectrum-propagation-loss-model.h>
#include <ns3/three-gpp-channel-model.h>
#include <ns3/buildings-channel-condition-model.h>
#include <ns3/nr-mac-scheduler-tdma-rr.h>
#include <ns3/bwp-manager-algorithm.h>

#include "ns3/uniform-planar-array.h"
#include "ns3/three-gpp-v2v-propagation-loss-model.h"



using namespace ns3;

/*
 * Enable the logs of the file by enabling the component "Cttc3gppChannelSimpleRan",
 * in this way:
 * $ export NS_LOG="Cttc3gppChannelSimpleRan=level_info|prefix_func|prefix_time"
 */
NS_LOG_COMPONENT_DEFINE ("Cttc3gppACapacityTest");


int
main (int argc, char *argv[])
{
  uint16_t gNbNum = 1;
  uint16_t ueNumPergNb = 5;
  bool enableUl = false;

  // set simulation time
  double simDuration=100.1;
  double appStartTime[ueNumPergNb];
  double appStop=simDuration-0.1;

  std::string scenario = "UMa_nLOS"; //scenario
  enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa_nLoS;
  double centralFrequency = 35.0e8;
  double bandwidth = 25e6;
  double hBS; //base station antenna height in meters,set later
  double hUT; //user antenna height in meters, set later
  double txPower = 49;//TR38.802,UMa,Below 6GHz
  uint16_t numerology = 1;
  double lambda = 1000;//Number of UDP packets per second
  uint32_t udpPacketSize = 1000;
  bool udpFullBuffer = true;
  std::string pattern = "DL|S|UL|UL|DL|DL|DL|DL|DL|DL|"; 
  
  uint8_t fixedMcs = 28;//fixing the MCS to a predefined value, both for downlink and uplink transmissions, separately
  bool useAdaptiveMcs = true;//two different AMC models for link adaptation:Error model-based,Shannon-based
  std::string errorModel = "ns3::NrEesmCcT1";
  uint64_t channelUpdatePeriod = 100;//ms, ThreeGppChannelModel, ChannelConditionModel
  /* if number of ue per gNB <=0,error and exit */
  NS_ASSERT (ueNumPergNb > 0);

  // Where we will store the output files.
  std::string simTag = "default";
  std::string outputDir = "./";

  /* parameters of p2p  link */
  const uint64_t p2pLinkBw   = 10e9;
  const uint32_t msP2pDelay  = 5;
  const uint32_t msQDelay = 50;

  CommandLine cmd;
  cmd.AddValue ("gNbNum",
                "The number of gNbs in multiple-ue topology",
                gNbNum);
  cmd.AddValue ("ueNumPergNb",
                "The number of UE per gNb in multiple-ue topology",
                ueNumPergNb);
  cmd.AddValue ("numerology",
                "The numerology to be used.",
                numerology);
  cmd.AddValue ("centralFrequencyBand1",
                "The system frequency",
                centralFrequency);
  cmd.AddValue ("bandwidth",
                "The system bandwidth",
                bandwidth);
  cmd.AddValue ("packetSize",
                "packet size in bytes",
                udpPacketSize);
  cmd.AddValue ("enableUl",
                "Enable Uplink",
                enableUl);
  cmd.AddValue ("txPower",
                "Tx power to be configured to gNB",
                txPower);
  cmd.AddValue ("simTag",
                "tag to be appended to output filenames to distinguish simulation campaigns",
                simTag);
  cmd.AddValue ("outputDir",
                "directory where to store simulation results",
                outputDir);
  cmd.AddValue ("lambda",
                "Number of UDP packets per second",
                lambda);
  cmd.AddValue ("udpFullBuffer",
                "Whether to set the full buffer traffic; if this parameter is set then the udpInterval parameter"
                "will be neglected",
                udpFullBuffer);
  cmd.AddValue ("fixedMcs",
                "The fixed MCS that will be used in this example if useFixedMcs is configured to true (1).",
                fixedMcs);
  cmd.AddValue ("useFixedMcs",
                "Whether to use adaptive mcs, mixed mcs is normally used for testing purposes",
                useAdaptiveMcs);
  cmd.Parse (argc, argv);

  //Config::SetDefault ("ns3::RadioBearerStatsCalculator::EpochDuration", TimeValue (MilliSeconds (10)))
  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (channelUpdatePeriod)));
  // Maximum Size of the Transmission Buffer (in Bytes);Initial Value:10240
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100000000));

  // set mobile device and base station antenna heights in meters, according to the chosen scenario
  if (scenario.compare ("RMa") == 0){
    hBS = 35;
    hUT = 1.5;
    scenarioEnum = BandwidthPartInfo::RMa;
  }
  else if (scenario.compare ("UMa_LOS") == 0){
    hBS = 25;
    hUT = 1.5;
    scenarioEnum = BandwidthPartInfo::UMa_LoS;
  }
  else if (scenario.compare ("UMa_nLOS") == 0){
    hBS = 25;
    hUT = 1.5;
    scenarioEnum = BandwidthPartInfo::UMa_nLoS;
  }
  else if (scenario.compare ("UMi-StreetCanyon") == 0){
    hBS = 10;
    hUT = 1.5;
    scenarioEnum = BandwidthPartInfo::UMi_StreetCanyon;
  }
  else if (scenario.compare ("InH-OfficeMixed") == 0){
    hBS = 3;
    hUT = 1;
    scenarioEnum = BandwidthPartInfo::InH_OfficeMixed;
  }
  else if (scenario.compare ("InH-OfficeOpen") == 0){
    hBS = 3;
    hUT = 1;
    scenarioEnum = BandwidthPartInfo::InH_OfficeOpen;
  }
  else{
    NS_ABORT_MSG ("Scenario not supported. Choose among 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', and 'InH-OfficeOpen'.");
  }

  
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
  nrHelper->SetBeamformingHelper (idealBeamformingHelper);
  nrHelper->SetEpcHelper (epcHelper);
  // Beamforming method
  idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));

  // Core latency
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  // Create one operational band containing one CC with one bandwidth part
  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1;

  // Create the configuration for the CcBwpHelper
  CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency, bandwidth,
                                                   numCcPerBand, scenarioEnum);
  bandConf.m_numBwp = 1; // 1 BWP per CC

  // By using the configuration created, it is time to make the operation band
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

  /*
   * Set the parameters which are common to all the nodes, like the
   * gNB transmit power or numerology.
   */
  nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (txPower));
  nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerology));
  nrHelper->SetGnbPhyAttribute("Pattern",StringValue(pattern));

  // Error Model: UE and GNB with same spectrum error model.
  nrHelper->SetUlErrorModel (errorModel);
  nrHelper->SetDlErrorModel (errorModel);

  // Configure scheduler
  nrHelper->SetSchedulerTypeId (NrMacSchedulerOfdmaPF::GetTypeId ());
  nrHelper->SetSchedulerAttribute ("FixedMcsDl", BooleanValue (!useAdaptiveMcs));
  nrHelper->SetSchedulerAttribute ("FixedMcsUl", BooleanValue (!useAdaptiveMcs));
  if (!useAdaptiveMcs == true){
    nrHelper->SetSchedulerAttribute ("StartingMcsDl", UintegerValue (fixedMcs));
    nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue (fixedMcs));
  }
  else {
    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
  }

  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (channelUpdatePeriod)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (true));


  // Antennas for all the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  // Antennas for all the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));




  nrHelper->InitializeOperationBand (&band);
  allBwps = CcBwpCreator::GetAllBwps ({band});


  // create base stations  and UE
  NodeContainer gNbNodes;
  NodeContainer ueNodes;
  gNbNodes.Create (gNbNum);
  ueNodes.Create (ueNumPergNb * gNbNum);


  // position the base stations
  Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator> ();
  //Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  gnbPositionAlloc->Add (Vector (400.0, 400.0, hBS));

  

  MobilityHelper gnbmobility;
  gnbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gnbmobility.SetPositionAllocator (gnbPositionAlloc);
  gnbmobility.Install (gNbNodes);

  // position the mobile terminals and enable the mobility
  MobilityHelper uemobility;
  uemobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                              "rho",DoubleValue(200.0),
                              "X",DoubleValue(200.0),
                              "Y",DoubleValue(200.0),
                              "Z",DoubleValue(hUT));
  //uemobility.SetPositionAllocator(uePositionAlloc);
  //uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("0.5s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", RectangleValue (Rectangle (0.0, 400.0, 0.0, 400.0)));
  uemobility.Install (ueNodes);

  // install nr net devices
  NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice (gNbNodes, allBwps);
  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes, allBwps);

  // When all the configuration is done, explicitly call UpdateConfig ()
  for (auto it = gnbNetDev.Begin (); it != gnbNetDev.End (); ++it) {
    DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
  }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it) {
    DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
  }

  // create the internet and install the IP stack on the UEs
  // get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (gNbNum);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (p2pLinkBw)));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msP2pDelay)));
  auto bufSizeP = std::max<uint32_t> (1, p2pLinkBw * msQDelay /(8000*1500));
  p2ph.SetQueue ("ns3::DropTailQueue",
                "MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, bufSizeP)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
   // Install the IP stack on the UEs
  internet.Install (ueNodes);

  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u){
    Ptr<Node> ueNode = ueNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueNetDev, gnbNetDev);

  uint16_t dlPort=5000;

  ApplicationContainer serverApps;
  // The sink will always listen to the specified ports
  UdpServerHelper dlPacketSinkHelper (dlPort);
  serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (0)));

  // The client, who is transmitting, is installed in the remote host
  UdpClientHelper dlClient;
  dlClient.SetAttribute ("RemotePort", UintegerValue (dlPort));
  dlClient.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));
  dlClient.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));

  if (udpFullBuffer){
    double bitRate = 100000000; // 75 Mbps will saturate the NR system of 20 MHz with the NrEesmIrT1 error model
      bitRate /= ueNumPergNb;    // Divide the cell capacity among UEs
      bitRate *=  bandwidth / 20e6;
      lambda = bitRate / static_cast<double> (udpPacketSize * 8);
  }
  dlClient.SetAttribute ("Interval", TimeValue (Seconds (1.0 / lambda)));

  // The bearer that will carry low latency traffic
  EpsBearer bearer (EpsBearer::NGBR_LOW_LAT_EMBB);

  Ptr<EpcTft> tft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf;
  dlpf.localPortStart = dlPort;
  dlpf.localPortEnd = dlPort;
  tft->Add (dlpf);

  /*
   * Let's install the applications!
   */
  ApplicationContainer clientApps;

  for (uint32_t i = 0; i < ueNodes.GetN (); ++i){
    Ptr<Node> ue = ueNodes.Get (i);
    Ptr<NetDevice> ueDevice = ueNetDev.Get (i);
    Address ueAddress = ueIpIface.GetAddress (i);

    // The client, who is transmitting, is installed in the remote host,
    // with destination address set to the address of the UE
    dlClient.SetAttribute ("RemoteAddress", AddressValue (ueAddress));
    clientApps.Add (dlClient.Install (remoteHost));

    // Activate a dedicated bearer for the traffic type
    nrHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, tft);
  }

  for (uint32_t i = 0; i < ueNumPergNb; i++) {
    //appStartTime[i]=0.001+(rand()%100)/20;//0.001--5s
    appStartTime[i]=0.1;
    //InstallTcp(remoteHostContainer.Get (0), ueNodes.Get (i),tcpServPort+i,appStartTime[i],appStop);
  }

  // start server and client apps
  serverApps.Start (Seconds (appStartTime[0]));
  clientApps.Start (Seconds (appStartTime[0]));
  serverApps.Stop (Seconds (appStop));
  clientApps.Stop (Seconds (appStop));


  // enable the traces provided by the nr module
  nrHelper->EnableTraces ();

  Simulator::Stop (Seconds (simDuration));
  Simulator::Run ();
  Simulator::Destroy ();

}
