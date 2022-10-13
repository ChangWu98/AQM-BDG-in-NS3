#ifndef AMLIRAN_SENDER_H
#define AMLIRAN_SENDER_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "amlrlcqueue.h"
#include "amliranTx.h"

namespace ns3
{
class AmliranSender:public ns3::Application
{
public:
	AmliranSender();
	~AmliranSender();
	void SetStream(uint16_t numStream);
	void InitialSetup(Ipv4Address dest_ip,uint16_t dest_port);
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	void Send(Ptr<Packet>packet);
	void sendProcess();
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
    Ptr<Socket> m_socket;
	uint64_t m_tick{10};//10us;
	EventId m_tickEvent;
	uint64_t m_seqNr[numStream];
	AmliranTx *m_amliranTx;
	bool m_running;
	uint16_t m_ssrc;
	int64_t m_nextCallN{-1};
	uint16_t m_mark;//1--Increase; 2--Decrease; 3--No action
};
}
#endif