#ifndef SCREAM_SENDER_H
#define SCREAM_SENDER_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "ns3/throughputRW-module.h"
#include "rtpqueue.h"
#include "videoenc.h"
#include "screamTx.h"
#include "ns3/mytrace-module.h"
namespace ns3
{
class ScreamSender:public ns3::Application
{
public:
	ScreamSender();
	~ScreamSender();
	void SetSourceFilePath(char*source);
	void InitialSetup(Ipv4Address dest_ip,uint16_t dest_port,throughputRW *thRW);
	VideoEnc* GetEncoder(){return m_videoEnc;}
	ScreamTx* GetTx(){return m_screamTx;}
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	void Send(Ptr<Packet>packet);
	void Process();
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
    Ptr<Socket> m_socket;
	int64_t m_videoTick{10};//20ms;
	int64_t m_nextFrameTime;
	int64_t m_tick{10};//10us;
	float m_frameRate{100};
	EventId m_tickEvent;
	RtpQueue *m_rtpQueue;
	VideoEnc *m_videoEnc;
	ScreamTx *m_screamTx;
	bool m_running;
	uint32_t m_ssrc;
	int m_nextCallN{-1};
	bool isFinishByteStatistic;
	int secfprintf;
	throughputRW *m_thput;
};
}
#endif
