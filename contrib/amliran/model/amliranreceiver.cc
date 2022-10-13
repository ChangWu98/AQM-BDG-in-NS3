#include "ns3/log.h"
#include "amliranreceiver.h"
#include "ns3/simulator.h"
#include <iostream>
#include <sstream>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("AmliranReceiver");
AmliranReceiver::AmliranReceiver(){
	m_amliranRx=new AmliranRx(0);
	m_rlcQueue = new AmlRlcQueue();
	m_rateCalQueue = new RateCalQueue();
	m_tbCalQueue = new TbCalQueue();
	//m_tracer = new AmliranTracer();
}
AmliranReceiver::~AmliranReceiver(){
	delete m_amliranRx;
	delete m_rlcQueue;
	delete m_rateCalQueue;
	delete m_tbCalQueue;
}
void AmliranReceiver::Bind(uint16_t port)
{
	m_port=port;  //uint16_t m_port
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());  //Ptr<Socket> m_socket
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);//Bind:Allocate a local endpoint for this socket.
    NS_ASSERT (ret == 0);
    //NSAssert(condition, desc)，condition为真时程序继续运行，为NO时抛出带有desc描述的异常信息
	m_socket->SetRecvCallback (MakeCallback (&AmliranReceiver::RecvPacket,this));  //this : m_socket

	m_tbSchedFp= fopen(tbSchedFile.c_str(),"r");
	NS_ASSERT_MSG(m_tbSchedFp,"Tb file open failed");
	fseek(m_tbSchedFp,0L,SEEK_SET);
	m_tracer.OpenTraceFile(string("amliran/"));
	m_amliranRx->SetDelayCallback(MakeCallback(&AmliranTracer::DelayTrace,&m_tracer));
	m_amliranRx->SetSchedRateCallback(MakeCallback(&AmliranTracer::schedRateTrace,&m_tracer));
	m_amliranRx->SetRecRateCallback(MakeCallback(&AmliranTracer::recRateTrace,&m_tracer));
	m_amliranRx->SetStreamRateCallback(MakeCallback(&AmliranTracer::streamRateTrace,&m_tracer));
}
void AmliranReceiver::RecvPacket(Ptr<Socket>socket)
{
	//if(!m_running){return;}
	Address remoteAddr;
	auto packet = m_socket->RecvFrom (remoteAddr);
	m_peerIp = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
	m_peerPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
	int64_t now_us=Simulator::Now().GetMicroSeconds();
	//uint64_t  time_ms=now_us/1000;
	AmliranHeader header;
	packet->RemoveHeader(header);
	rtp_hdr_t rtp;
	header.GetHeaderPayload((void*)&rtp,sizeof(rtp));
	uint16_t ssrc=rtp.ssrc;
	uint64_t seqNr=rtp.seq_number;
	uint32_t size=packet->GetSize();
	uint32_t tmsp=rtp.timestamp;
	void *rtpPacket = 0;
	//cout<<"Time: "<<now_us<<" ssrc: "<<ssrc<<" seqNr: "<<seqNr<<endl;
	m_mark = m_amliranRx->upfReceive(now_us,rtpPacket,ssrc,seqNr,size,tmsp,m_rateCalQueue);
	//m_mark = m_amliranRx->ifMark(ssrc,seqNr,now_us);
	m_rlcQueue->push(rtpPacket,size,ssrc,seqNr,m_mark,tmsp,now_us);
	//cout<<"rlc Queue push successfully!!"<<ssrc<<endl;

	if(m_first){
		m_first=false;
		Time next=MicroSeconds(2);
		m_schedEvent=Simulator::Schedule(next,&AmliranReceiver::rlcProcess,this);
	}


	
	// m_amliranRx->receive(time_us,0,ssrc,size,seqNr);
	// NS_LOG_INFO("recv seq "<<rtp.seq_number);
	// if(m_first)
	// {
	// 	m_first=false;
	// 	Time next=MilliSeconds(m_rtcpFbInterval);
	// 	m_feedbackEvent=Simulator::Schedule(next,&AmliranReceiver::Process,this); //EventId m_feedbackEvent
	// 	//在当前时间点+Time const& delay 这一时间段内调度ScreamReceiver::Process事件，后面为传递给调用函数的参数
	// }
}
void AmliranReceiver::rlcProcess(){
	int64_t now_us=Simulator::Now().GetMicroSeconds();
	int64_t retSchedVal = -1;//(us)
	//cout<<"rlc processing at "<<now_us<<endl;
	if(m_schedEvent.IsExpired()){
		if (now_us>=m_nextCallN){
			retSchedVal = m_amliranRx->isOkToSched(now_us,m_numPDeliver,m_tbSchedFp,m_rlcQueue,m_rateCalQueue,m_tbCalQueue);       //RLC queue--->receiver,OK? -->m_numPDeliver
			//cout<<"isOkToSched success, returned. retSchedVal is "<<retSchedVal<<endl;
			if(retSchedVal > 0) {
				//cout<<"rlc process after time "<<retSchedVal<<endl;
				m_nextCallN = now_us+retSchedVal;
			}
		}
		if(retSchedVal == 0){
			//cout<<"schedule packet once at "<<now_us<<endl;
			/* Packet can be delivered to receiver and feedback*/
			NS_LOG_INFO(now_us<<" schedule packet once");
			int64_t tmsp[numStream];//highest timestamp of packet has received of every stream
			uint64_t seqNr[numStream];//highest sequence number of packet has received of every stream
        	int16_t incPac[numStream];//number of packets with "increase", negtive with "decrease"
			for(int i=0; i<numStream; i++){
				tmsp[i]=0;
				seqNr[i]=0;
				incPac[i]=0;
			}
			//uint16_t ssrc_t;//ssrc of first packet within m_numPDeliver packets
    		//uint64_t seqNr_t;//seqNr of first packet within m_numPDeliver packets
			//int64_t ts_t;//timestamp of first packet within m_numPDeliver packets
			if (m_amliranRx->getSchedule(now_us, m_numPDeliver, m_rlcQueue, seqNr, incPac, tmsp)){
				rtcp_common_t rtcp;
		    	memset(&rtcp,0,sizeof(rtcp));//&rtcp+sizeof(rtcp)地址空间初始化为0
		    	rtcp.length=sizeof(amliran_feedback_t);
		    	AmliranHeader header;
		    	header.SetMid(AmliranProto::SC_RTCP);
		    	header.SetHeaderPayload((void*)&rtcp,sizeof(rtcp));
		    	amliran_feedback_t feedback;
				for(int i=0; i<numStream; i++){
					feedback.timestamp[i] = tmsp[i];
					feedback.seqNr[i]=seqNr[i];
					feedback.incPac[i]=incPac[i];
				}
		    	Ptr<Packet> p=Create<Packet>((uint8_t*)&feedback,sizeof(feedback));
			 	p->AddHeader(header);
		    	NS_LOG_INFO(now_us<<"feedback "<<incPac[0]<<" "<<incPac[1]);
		    	Send(p);

				retSchedVal=m_amliranRx->addDelivered(now_us,m_tbSchedFp);
			}
		}
		if(retSchedVal>0){
			m_nextCallN = now_us + retSchedVal;
		}
		else{
			//m_nextCallN = now_us + 10;
			retSchedVal = 1;
		}
		Time next=MicroSeconds((uint64_t)retSchedVal);
		Simulator::Schedule(next,&ns3::AmliranReceiver::rlcProcess,this);
	}
}

void AmliranReceiver::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
	//cout<<"fbFlag: "<<fbFlag<<endl;
}
void AmliranReceiver::StartApplication()
{
	m_running=true;
}
void AmliranReceiver::StopApplication()
{
	m_running=false;
	m_schedEvent.Cancel();
	fclose(m_tbSchedFp);
	m_tracer.CloseTraceFile();
	
}
}
