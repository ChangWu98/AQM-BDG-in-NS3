#ifndef AMLIRAN_RECEIVER_H
#define AMLIRAN_RECEIVER_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "amliranRx.h"
#include "amlrlcqueue.h"
#include "amliran-header.h"
#include "amliran.h"
#include "ns3/amlirantrace-module.h"
namespace ns3
{	//char buf_t[FILENAME_MAX];
	string tbSched_path="/home/ubuntu-ns3/ns-allinone-3.35/ns-3.35/VideoSource/";
	string tbSchedFile = tbSched_path+"UE_Tb_Size.txt";
class AmliranReceiver:public ns3::Application
{
public:
	AmliranReceiver();
	~AmliranReceiver();
	void Bind(uint16_t port);	
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	void Send(Ptr<Packet>packet);
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
	AmliranRx *m_amliranRx;
	Ptr<Socket> m_socket;
	uint16_t m_port;
	bool  m_running{false};
	bool m_first{true};
	



	//uint64_t m_schedInterval{100};//100us;
	EventId m_schedEvent;
    AmlRlcQueue* m_rlcQueue;
	RateCalQueue* m_rateCalQueue;
	TbCalQueue* m_tbCalQueue;
	uint16_t m_ssrc;
	int64_t m_nextCallN{-1};
	uint16_t m_mark;//1--Increase; 2--Decrease; 3--No action
	uint16_t m_numPDeliver;//number of packet can be delivered
	void rlcProcess();
	FILE *m_tbSchedFp=nullptr;
	
	AmliranTracer m_tracer;

	
};
}
#endif
