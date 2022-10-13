#include "amliransender.h"
#include "amliran-header.h"
#include "ns3/log.h"
#include "amliran.h"

using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("AmliranSender");
AmliranSender::AmliranSender()
{
	m_running=false;
	m_amliranTx = new AmliranTx();

}
AmliranSender::~AmliranSender()
{
	delete m_amliranTx;

}
void AmliranSender::SetStream(uint16_t numStream)
{
	
}
void AmliranSender::InitialSetup(Ipv4Address dest_ip,uint16_t dest_port)
{ 
	m_peerIp=dest_ip;
	m_peerPort=dest_port;
	m_ssrc=0;
}
void AmliranSender::StartApplication()
{	
	double strStartRate = 0;
	if (numStream == 1){strStartRate = sumStartRate;}
	else {strStartRate = sumStartRate/3;}
	for(uint16_t no=0; no<numStream; no++){
		m_amliranTx->registerNewStream(m_ssrc+no, 1.8e6f, (no+1)*strStartRate, 4*8192e3f);
		//m_amliranTx->registerNewStream(m_ssrc+no, 1.8e6f, 15e6f, 4*8192e3f);
		m_seqNr[no]=1;
	}

	if(m_socket==NULL)
	{
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
	}
	m_socket->SetRecvCallback (MakeCallback(&AmliranSender::RecvPacket,this));
	m_running=true;
	m_tickEvent=Simulator::ScheduleNow(&AmliranSender::sendProcess,this);
}
void AmliranSender::StopApplication()
{
	m_running=false;
	m_tickEvent.Cancel();
}
void AmliranSender::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,can't send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
}

void AmliranSender::sendProcess()
{
	int64_t now_us=Simulator::Now().GetMicroSeconds();
	int32_t retSendVal = -1;//retSendVal(us)
	if(m_tickEvent.IsExpired())
	{
		if (now_us>=m_nextCallN){	
			retSendVal = m_amliranTx->isOkToTransmit(now_us, m_ssrc);//server---->RLC queue,OK? -->m_ssrc;
			if(retSendVal > 0){
				m_nextCallN = now_us+retSendVal;
			}
		}
        if (retSendVal == 0) {
            /*
            * RTP packet can be transmitted to receiver
            */
		   	//cout<<"send stream "<<m_ssrc<<" immmediately!!!!!!!!"<<now_us<<endl;
            void *rtpPacket = 0;
            int size = pacSize;;
            uint64_t seqNr=m_seqNr[m_ssrc];
			rtp_hdr_t rtp;
            rtp.seq_number=seqNr;
            rtp.ssrc=m_ssrc;
            rtp.timestamp=(uint32_t)now_us;
			rtp.markbit=0b11;
			//cout<<rtp.timestamp<<" "<<rtp.ssrc<<" "<<rtp.seq_number<<" "<<rtp.markbit<<endl;
            AmliranHeader header;
            header.SetMid(AmliranProto::SC_RTP);
        	header.SetHeaderPayload((void*)&rtp,sizeof(rtp));
        	Ptr<Packet> p=Create<Packet>(size);
        	p->AddHeader(header);
        	Send(p);
            //NS_LOG_INFO(now<<" send "<<seqNr);
			m_seqNr[m_ssrc]++;

            retSendVal = m_amliranTx->addTransmitted(now_us, m_ssrc, size, seqNr);
            m_nextCallN =now_us+retSendVal;				
		}
		//cout<<"retSendVal: "<<retSendVal<<endl;
    }
		
	if(retSendVal>=0)
		m_nextCallN = now_us + retSendVal;
	//else
		//retSendVal += 100;
		//m_nextCallN = now_us + 100;
	Time next=MicroSeconds((uint64_t)retSendVal);
	Simulator::Schedule(next,&AmliranSender::sendProcess,this);
}

void AmliranSender::RecvPacket(Ptr<Socket>socket){
	if(!m_running){return;}
	Address remoteAddr;
	auto p= m_socket->RecvFrom (remoteAddr);
	AmliranHeader header;
	p->RemoveHeader(header);
	rtcp_common_t rtcp;
	header.GetHeaderPayload((void*)&rtcp,sizeof(rtcp));
	uint8_t buffer[128];
	p->CopyData ((uint8_t*)&buffer, p->GetSize ());
	amliran_feedback_t *feedback;
	feedback=(amliran_feedback_t*)buffer;
	int64_t now_us=Simulator::Now().GetMicroSeconds();
    //NS_LOG_INFO("incoming feedback"<<feedback->aseq<<" "<<feedback->timestamp);
	int64_t txTimestamp[numStream];
	uint64_t seqNr[numStream];
    int16_t incPac[numStream];
	for(int i=0; i<numStream; i++){
		txTimestamp[i] = feedback->timestamp[i];
		incPac[i] = feedback->incPac[i];
		seqNr[i] = feedback->seqNr[i];
	}
	m_amliranTx->incomingFeedback(now_us,txTimestamp,seqNr,incPac);
}
}
