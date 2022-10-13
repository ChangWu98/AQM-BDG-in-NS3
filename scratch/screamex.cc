#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/scream-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/mytrace.h"
#include "ns3/throughputRW-module.h"
#include <stdarg.h>
#include <string>
#include <math.h>

#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"

#include "ns3/nr-module.h"
#include "ns3/antenna-module.h"
#include "ns3/netanim-module.h"


NS_LOG_COMPONENT_DEFINE("SCREAM_EXAMPLE");
using namespace ns3;
using namespace std;
const uint32_t TOPO_DEFAULT_BW     = uint32_t(1e9);    // in bps: ueNum*10Mbps 
const uint32_t TOPO_DEFAULT_PDELAY =      1;    // in ms:   1ms
const uint32_t TOPO_DEFAULT_QDELAY =     100;    // in ms:  100ms
const uint32_t DEFAULT_PACKET_SIZE = 1500;

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
      device->SetDataRate(DataRate((uint64_t)(15.0*1e6)));
    }
		Time next=Seconds(10.0);
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
    }
    int64_t now=Simulator::Now().GetMilliSeconds();
    if(now>=19900){cout<<"sumLinkCapacity: "<<m_sumLinkCapacity<<endl;}
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
};



static void InstallScream(
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float startTime,
                         float stopTime,
                         char*source,
                         MyTracer*tracer,
                         throughputRW *thRW
                         )
{
	Ptr<ScreamSender> sendApp=CreateObject<ScreamSender>();
	Ptr<ScreamReceiver> recvApp=CreateObject<ScreamReceiver>();
  Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
  Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
  sendApp->InitialSetup(receiverIp,port,thRW);
	sendApp->SetSourceFilePath(source);
  sender->AddApplication (sendApp);
  receiver->AddApplication (recvApp);
  recvApp->Bind(port);

	VideoEnc *encoder=sendApp->GetEncoder();
	if(encoder)
	{
	encoder->SetRateCallback(MakeCallback(&MyTracer::RateTrace,tracer));
	}
	recvApp->SetDelayCallback(MakeCallback(&MyTracer::DelayTrace,tracer));
  ScreamTx *screamtx = sendApp->GetTx();
  if(screamtx){
    screamtx->SetThroughputCallback(MakeCallback(&MyTracer::ThroughputTrace,tracer));
  }
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));

    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));

}



int main(int argc, char *argv[])
{
 // enable logging or not
  bool logging = true;
  if (logging)
    {
      LogComponentEnable("SCREAM_EXAMPLE", LOG_LEVEL_ALL);
	    //LogComponentEnable("ScreamReceiver", LOG_LEVEL_ALL);
	    //LogComponentEnable("ScreamSender",LOG_LEVEL_ALL);
      //LogComponentEnable("VideoEnc",LOG_LEVEL_ALL);
      LogComponentEnable("ScreamTx",LOG_LEVEL_ALL);
    }

  uint16_t gNbNum = 1;
  uint32_t ueNumPergNb = 1;
  // set simulation time
  double simDuration=20.2;
  float appStart[ueNumPergNb];
  float appStop=simDuration-0.1;


  std::string scenario = "UMa_LOS"; //scenario
  enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa_LoS;
  double centralFrequency = 35e8;// central frequency
  double bandwidth = 100e6;//bandwidth

  double hBS; //base station antenna height in meters
  double hUT; //user antenna height in meters
  double txPower = 69;//TR38.802,UMa,Below 6GHz
 
 /*  Cc separation,automatic,divides the bandwidth
  in a given number of equally-sized contiguous CCs. */
  bool contiguousCc = true;
  uint16_t numerology = 1;
  /* TDD  patterm,Pattern can be e.g. "DL|S|UL|UL|DL|DL|S|UL|UL|DL|" */
  std::string pattern = "DL|S|UL|UL|DL|DL|DL|DL|DL|DL|"; 
 // double lambda = 1000;
 // uint32_t udpPacketSize = 1000;
 // bool udpFullBuffer = true;
  uint8_t fixedMcs = 28;//fixing the MCS to a predefined value, both for downlink and uplink transmissions, separately
  bool useAdaptiveMcs = true;//two different AMC models for link adaptation:Error model-based,Shannon-based
  std::string errorModel = "ns3::NrEesmCcT1";

  uint16_t servPort=5432;
  //uint16_t tcpServPort=5000;
  
  /* if number of ue per gNB <=0,error and exit */
  NS_ASSERT (ueNumPergNb > 0);

  /* parameters of p2p  link */
  const uint64_t linkBw   = TOPO_DEFAULT_BW;
  const uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
  const uint32_t msQDelay = TOPO_DEFAULT_QDELAY;


  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simDuration", 
                "Total duration of the simulation",
                 simDuration);
  cmd.AddValue ("ueNumPergNb", 
                "Number of  UE Node", 
                ueNumPergNb);
  cmd.AddValue ("scenario",
                "The scenario for the simulation. Choose among 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', 'InH-OfficeOpen'.",
                scenario);
  cmd.AddValue ("errorModelType",
                "Error model type: ns3::NrEesmCcT1, ns3::NrEesmCcT2, ns3::NrEesmIrT1, ns3::NrEesmIrT2, ns3::NrLteMiErrorModel",
                errorModel);
  cmd.Parse (argc, argv);


  // Maximum Size of the Transmission Buffer (in Bytes);Initial Value:10240
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100000000));

  Config::SetDefault ("ns3::NrAmc::ErrorModelType", TypeIdValue (TypeId::LookupByName (errorModel)));
  Config::SetDefault ("ns3::NrAmc::AmcModel", EnumValue (NrAmc::ErrorModel));  // NrAmc::ShannonModel or NrAmc::ErrorModel


  // to save a template default attribute file run it like this:
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save --ns3::ConfigStore::FileFormat=RawText" --run scratch/screamex
  //
  // to load a previously created default attribute file
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load --ns3::ConfigStore::FileFormat=RawText" --run scratch/screamex
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // Parse again so you can override default values from the command line
  cmd.Parse (argc, argv);


  // set mobile device and base station antenna heights in meters, according to the chosen scenario
  if (scenario.compare ("RMa") == 0)
    {
      hBS = 35;
      hUT = 1.5;
      scenarioEnum = BandwidthPartInfo::RMa;
    }
  else if (scenario.compare ("UMa_LOS") == 0)
    {
      hBS = 25;
      hUT = 1.5;
      scenarioEnum = BandwidthPartInfo::UMa_LoS;
    }
  else if (scenario.compare ("UMi-StreetCanyon") == 0)
    {
      hBS = 10;
      hUT = 1.5;
      scenarioEnum = BandwidthPartInfo::UMi_StreetCanyon;
    }
  else if (scenario.compare ("InH-OfficeMixed") == 0)
    {
      hBS = 3;
      hUT = 1;
      scenarioEnum = BandwidthPartInfo::InH_OfficeMixed;
    }
  else if (scenario.compare ("InH-OfficeOpen") == 0)
    {
      hBS = 3;
      hUT = 1;
      scenarioEnum = BandwidthPartInfo::InH_OfficeOpen;
    }
  else
    {
      NS_ABORT_MSG ("Scenario not supported. Choose among 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', and 'InH-OfficeOpen'.");
    }



  /*
   * Create NR simulation helpers
   */
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject <IdealBeamformingHelper> ();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
  nrHelper->SetBeamformingHelper (idealBeamformingHelper);
  nrHelper->SetEpcHelper (epcHelper);

  // Configure ideal beamforming method
  idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));

  // Core latency
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));


 /*
   * Spectrum division. We create one operation band with one component carrier
   * (CC) which occupies the whole operation band bandwidth. The CC contains a
   * single Bandwidth Part (BWP). This BWP occupies the whole CC band.
   * Both operational bands will use the StreetCanyon channel modeling.
   */
  BandwidthPartInfoPtrVector allBwps;
  
  CcBwpCreator ccBwpCreator;
  OperationBandInfo band;

  if(contiguousCc==true)
  {
    /*
       * CC band configuration n257F (NR Release 15): four contiguous CCs of
       * 400MHz at maximum. In this automated example, each CC contains a single
       * BWP occupying the whole CC bandwidth.
       *
       * The configured spectrum division is:
       * ----------------------------- Band --------------------------------
       * ------CC0------|------CC1-------|-------CC2-------|-------CC3-------
       * ------BWP0-----|------BWP0------|-------BWP0------|-------BWP0------
       */
    const uint8_t numCcPerBand = 1;  // in this example, both bands have a single CC
   /*       * Hence, the configured spectrum is:
   *
   * |---------------Band---------------|
   * |---------------CC-----------------|
   * |---------------BWP----------------|
    */
    CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency,
                                                  bandwidth,
                                                  numCcPerBand,
                                                  scenarioEnum);
    bandConf.m_numBwp = 1; // 1 BWP per CC
    // By using the configuration created, it is time to make the operation bands
    band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);
  }
  
  /*
   * Initialize channel and pathloss, plus other things inside band1. 
   */
  nrHelper->InitializeOperationBand (&band);
  allBwps = CcBwpCreator::GetAllBwps ({band});
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

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
  nrHelper->SetSchedulerTypeId (NrMacSchedulerTdmaPF::GetTypeId ());
  nrHelper->SetSchedulerAttribute ("FixedMcsDl", BooleanValue (!useAdaptiveMcs));
  nrHelper->SetSchedulerAttribute ("FixedMcsUl", BooleanValue (!useAdaptiveMcs));

    if (!useAdaptiveMcs == true)
    {
      nrHelper->SetSchedulerAttribute ("StartingMcsDl", UintegerValue (fixedMcs));
      nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue (fixedMcs));
    }
    else
    {
      // Both DL and UL AMC will have the same model behind.
      nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
      nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    }

  // Antennas for all the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  // Antennas for all the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  uint32_t bwpId = 0;
  // gNb routing between Bearer and bandwidh part
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("NGBR_LOW_LAT_EMBB", UintegerValue (bwpId));
  // Ue routing between Bearer and bandwidth part
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("NGBR_LOW_LAT_EMBB", UintegerValue (bwpId));


  // create base stations  and UE
  NodeContainer gNbNodes;
  NodeContainer ueNodes;
  gNbNodes.Create (gNbNum);
  ueNodes.Create (ueNumPergNb * gNbNum);

  // position the base stations
  Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator> ();
  gnbPositionAlloc->Add (Vector (100.0, 100.0, hBS));

  MobilityHelper gnbmobility;
  gnbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gnbmobility.SetPositionAllocator (gnbPositionAlloc);
  gnbmobility.Install (gNbNodes);

  // position the mobile terminals and enable the mobility
  MobilityHelper uemobility;
  uemobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                              "rho",DoubleValue(120.0),
                              "X",DoubleValue(120.0),
                              "Y",DoubleValue(120.0),
                              "Z",DoubleValue(hUT));
// uemobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
//                               "Theta", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=6.2830]"),
//                               "Rho",StringValue ("ns3::UniformRandomVariable[Min=00.0|Max=50]"),
//                               "X",DoubleValue(0.0),
//                               "Y",DoubleValue(0.0),
//                               "Z",DoubleValue(hUT));
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                            "Mode", StringValue ("Time"),
  //                            "Time", StringValue ("0.5s"),
  //                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
  //                            "Bounds", RectangleValue (Rectangle (0.0, 200.0, 0.0, 200.0)));
  uemobility.Install (ueNodes);

  // install nr net devices
  NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice (gNbNodes, allBwps);
  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes, allBwps);

  int64_t randomStream = 1;
  randomStream += nrHelper->AssignStreams (gnbNetDev, randomStream);
  randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);


  // When all the configuration is done, explicitly call UpdateConfig ()
  for (auto it = gnbNetDev.Begin (); it != gnbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
    {
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
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (linkBw)));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
  auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE/1500, linkBw * msQDelay /(8000*150));
  p2ph.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (std::to_string(bufSize)+"p"));
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
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach UE to eNodeB, side effect: the default EPS bearer will be activatedGet(0)
  nrHelper->AttachToClosestEnb (ueNetDev, gnbNetDev);


  char screamSourcePath[61][FILENAME_MAX];
	MyTracer screamTracer[ueNumPergNb];
  throughputRW *thput=new throughputRW;
  thput->thrwInitialize(ueNumPergNb);
  // Install SCReAM to remoteHostNodes and ueNodes
  for (uint32_t i = 0; i < ueNumPergNb; i++)
  {
    appStart[i]=0.1;
    string screamSourcePathStr="/home/ubuntu-ns3/ns-allinone-3.35/ns-3.35/VideoSource/cut" + to_string((i%6)+1)+".txt";
    strcpy(screamSourcePath[i],screamSourcePathStr.c_str());
    //screamTracePath=screamTracePath.append("/screamUser").append(to_string(i+1));
    screamTracer[i].OpenTraceFile("scream/");
    InstallScream(remoteHostContainer.Get (0), ueNodes.Get (i),servPort+i,appStart[i],appStop,screamSourcePath[i],&screamTracer[i],thput);
  } 

  Ptr<NetDevice> netDevice=remoteHostContainer.Get(0)->GetDevice(1);
	ChangeBw change(netDevice);
	change.Start();
  Simulator::Stop (Seconds(simDuration));
  Simulator::Run ();
  Simulator::Destroy();
	return 0;
}
