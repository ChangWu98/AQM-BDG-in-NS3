#include "ns3/log.h"
#include "screamreceiver.h"
#include "ns3/simulator.h"
#include <iostream>
#include <sstream>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ScreamReceiver");
ScreamReceiver::ScreamReceiver(){
	m_screamRx=new ScreamRx(0);
}
ScreamReceiver::~ScreamReceiver(){
	delete m_screamRx;
}
void ScreamReceiver::Bind(uint16_t port)
{
	m_port=port;  //uint16_t m_port
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());  //Ptr<Socket> m_socket
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);//Bind:Allocate a local endpoint for this socket.
    NS_ASSERT (ret == 0);
    //NSAssert(condition, desc)，condition为真时程序继续运行，为NO时抛出带有desc描述的异常信息
    m_socket->SetRecvCallback (MakeCallback (&ScreamReceiver::RecvPacket,this));  //this : m_socket
}
void ScreamReceiver::RecvPacket(Ptr<Socket>socket)
{
	if(!m_running){return;}
	Address remoteAddr;
	auto packet = m_socket->RecvFrom (remoteAddr);
	m_peerIp = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
	m_peerPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
	int64_t now=Simulator::Now().GetMilliSeconds();
	int64_t now_us=Simulator::Now().GetMicroSeconds();
	uint64_t  time_us=now*1000;
	ScreamHeader header;
	packet->RemoveHeader(header);
	rtp_hdr_t rtp;
	header.GetHeaderPayload((void*)&rtp,sizeof(rtp));
	uint32_t ssrc=rtp.ssrc;
	uint16_t seqNr=rtp.seq_number;
	uint32_t size=packet->GetSize();
	uint32_t tmp=rtp.timestamp;
	if(now > 10000){
		float gap=(now_us*1e-3)-tmp;
		if(!m_delayCb.IsNull()){m_delayCb(gap-1.0);}
	}
	
	m_screamRx->receive(time_us,0,ssrc,size,seqNr);
	NS_LOG_INFO("recv seq "<<rtp.seq_number);
	if(m_first)
	{
		m_first=false;
		Time next=MilliSeconds(m_rtcpFbInterval);
		m_feedbackEvent=Simulator::Schedule(next,&ScreamReceiver::Process,this); //EventId m_feedbackEvent
		//在当前时间点+Time const& delay 这一时间段内调度ScreamReceiver::Process事件，后面为传递给调用函数的参数
	}
}
void ScreamReceiver::Process()
{
	int64_t now=Simulator::Now().GetMilliSeconds();
	uint64_t time_us=now*1000;
	bool isFeedback=false;
	if(m_feedbackEvent.IsExpired())//判断词EventId 是否过期，过期返回true
	{
		isFeedback |= m_screamRx->isFeedback(time_us);
		//isFeedback() Returns true new RTP packets are received and SCReAM RTCP feedback can be transmitted
		 if (isFeedback)
		 {
			 uint32_t ssrc=0;
			 uint32_t rxTimestamp=0;
		     uint16_t aseqNr=0;
		     uint64_t aackVector=0;
	         uint16_t ecnCeMarkedBytes=0;
		     if (m_screamRx->getFeedback(time_us, ssrc, rxTimestamp, aseqNr, aackVector,ecnCeMarkedBytes))
		     {
		    	 rtcp_common_t rtcp;
		    	 memset(&rtcp,0,sizeof(rtcp));//&rtcp+sizeof(rccp)地址空间初始化为0
		    	 rtcp.length=sizeof(scream_feedback_t);
		    	 ScreamHeader header;
		    	 header.SetMid(ScreamProto::SC_RTCP);
		    	 header.SetHeaderPayload((void*)&rtcp,sizeof(rtcp));
		    	 scream_feedback_t feedback;
		    	 feedback.ack_vec=aackVector;
    			std::stringstream stream;
			stream << std::hex <<aackVector;
			std::string result( stream.str() );
			NS_LOG_INFO("ACK "<<result);
		    	 feedback.aseq=aseqNr;
		    	 feedback.ssrc=ssrc;
		    	 feedback.timestamp=rxTimestamp;
			 feedback.ecn_bytes=ecnCeMarkedBytes;
		    	 Ptr<Packet> p=Create<Packet>((uint8_t*)&feedback,sizeof(feedback));
			 p->AddHeader(header);
		    	 NS_LOG_INFO(now<<"feedback "<<aseqNr<<" "<<rxTimestamp);
		    	 Send(p);

		     }
		 }
		Time next=MilliSeconds(m_rtcpFbInterval);
		m_feedbackEvent=Simulator::Schedule(next,&ScreamReceiver::Process,this);
	}
}
void ScreamReceiver::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void ScreamReceiver::StartApplication()
{
	m_running=true;
}
void ScreamReceiver::StopApplication()
{
	m_running=false;
	m_feedbackEvent.Cancel();
}
}
