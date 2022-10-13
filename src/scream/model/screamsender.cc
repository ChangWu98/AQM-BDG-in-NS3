#include "screamsender.h"
#include "ns3/throughputRW-module.h"
#include "scream-header.h"
#include "ns3/log.h"
using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ScreamSender");
ScreamSender::ScreamSender()
{
	m_running=false;
	m_screamTx = new ScreamTx();
	m_rtpQueue = new RtpQueue();
	m_videoEnc=nullptr;
	isFinishByteStatistic=false;
	secfprintf=9;
	//m_ssrc=1111;
}
ScreamSender::~ScreamSender()
{
	delete m_screamTx;
	delete m_rtpQueue;
	delete m_videoEnc;
}
void ScreamSender::SetSourceFilePath(char*source)
{
	m_videoEnc=new VideoEnc(m_rtpQueue,m_frameRate,source);
	m_screamTx->registerNewStream(m_rtpQueue, m_ssrc, 1.0f, 1024e3f, 8*1024e3f, 4*8192e6f,10e6f);//, 0.1, 0.2f, 0.1f, 0.9f,0.95f);
}
void ScreamSender::InitialSetup(Ipv4Address dest_ip,uint16_t dest_port,throughputRW *thRW)
{ 
	m_peerIp=dest_ip;
	m_peerPort=dest_port;
	m_ssrc=dest_port-5431;
	m_thput=thRW;
}
void ScreamSender::StartApplication()
{
	if(m_socket==NULL)
	{
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
	}
	m_socket->SetRecvCallback (MakeCallback(&ScreamSender::RecvPacket,this));
	m_running=true;
	m_nextFrameTime=Simulator::Now().GetMilliSeconds();
	m_tickEvent=Simulator::ScheduleNow(&ScreamSender::Process,this);
}
void ScreamSender::StopApplication()
{
	m_running=false;
	m_tickEvent.Cancel();
}
void ScreamSender::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,can't send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void ScreamSender::Process()
{
	int64_t now=Simulator::Now().GetMilliSeconds();
	int64_t now_us = Simulator::Now().GetMicroSeconds();
	float time_s=Simulator::Now().GetSeconds();
	uint64_t time_us=now_us;
	float retVal = -1.0;//retVal(us)
	if(m_tickEvent.IsExpired())
	{
		if(now>=m_nextFrameTime)//generate a frame
		{
			m_nextFrameTime=now+m_videoTick;
			NS_LOG_INFO(now<<" generate a frame");
            m_videoEnc->setTargetBitrate(m_screamTx->getTargetBitrate(m_ssrc));
            int bytes = m_videoEnc->encode(time_s);
            m_screamTx->newMediaFrame(time_us, m_ssrc, bytes);
            retVal = m_screamTx->isOkToTransmit(time_us, m_ssrc);
		}
		if (now_us>=m_nextCallN)
		{
			retVal = m_screamTx->isOkToTransmit(time_us, m_ssrc);
			if(retVal>0)
			{
				//cout<<"ret: "<<retVal<<endl;
				m_nextCallN=now_us+retVal;
				NS_LOG_INFO("ret "<<retVal);
			}
		}
        if (retVal == 0) {
			//cout<<"now_us: "<<now_us<<endl;
            /*
            * RTP packet can be transmitted
            */
            void *rtpPacket = 0;
            int size;
            uint16_t seqNr;
            m_rtpQueue->sendPacket(rtpPacket, size, seqNr);
            retVal = m_screamTx->addTransmitted(time_us, m_ssrc, size, seqNr);
            m_nextCallN =now_us+retVal;
            rtp_hdr_t rtp;
            rtp.seq_number=seqNr;
            rtp.ssrc=m_ssrc;
            rtp.timestamp=(uint32_t)now;
            ScreamHeader header;
            header.SetMid(ScreamProto::SC_RTP);
        	header.SetHeaderPayload((void*)&rtp,sizeof(rtp));
        	Ptr<Packet> p=Create<Packet>(size);
        	p->AddHeader(header);
        	Send(p);
            //NS_LOG_INFO(now<<" send "<<seqNr);
			// if(now*1e-3>=15.1 && !isFinishByteStatistic){
				
			// if(int(now*1e-3)==secfprintf){
			// 	//int accBytesTransmitted=m_screamTx->getByteTranStatistics();
			// 	int accByteReceived=m_screamTx->getByteReceiveStatistics();
			// 	m_thput->addSingleRate(int(now*1e-3),m_ssrc,accByteReceived*8);
			// 	// fprintf(fopen("output/ScreamByteLog.txt", "a+"),"%8d %8d %8d %8.3f\n",
			// 	//  	int(now*1e-3),m_ssrc,accByteReceived*8,
			// 	//  	float(accBytesTransmitted-accByteReceived)/accBytesTransmitted);
			// 	if (m_thput->isOkWriteToFile(int(now*1e-3)))
			// 		m_thput->writeToFile();
			// 	secfprintf+=1;
			// 	m_screamTx->getByteTranClear();
			// 	m_screamTx->getByteReceiveClear();
			// }
        }
		//cout<<"retVal: "<<retVal<<endl;
		Time next=MicroSeconds(m_tick);
		Simulator::Schedule(next,&ScreamSender::Process,this);
	}
}
void ScreamSender::RecvPacket(Ptr<Socket>socket)
{
	if(!m_running){return;}
	Address remoteAddr;
	auto p= m_socket->RecvFrom (remoteAddr);
	ScreamHeader header;
	p->RemoveHeader(header);
	rtcp_common_t rtcp;
	header.GetHeaderPayload((void*)&rtcp,sizeof(rtcp));
	uint8_t buffer[128];
	p->CopyData ((uint8_t*)&buffer, p->GetSize ());
	scream_feedback_t *feedback;
	feedback=(scream_feedback_t*)buffer;
	int64_t now=Simulator::Now().GetMilliSeconds();
	uint64_t time_us=now*1000;
        //NS_LOG_INFO("incoming feedback"<<feedback->aseq<<" "<<feedback->timestamp);
	uint32_t rxTimestamp=feedback->timestamp;
	m_screamTx->incomingFeedback(time_us,m_ssrc,rxTimestamp,feedback->aseq,
			feedback->ack_vec,feedback->ecn_bytes);
}
}

