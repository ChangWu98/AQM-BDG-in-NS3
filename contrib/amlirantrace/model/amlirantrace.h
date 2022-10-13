/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef AMLIRANTRACE_H
#define AMLIRANTRACE_H
#include <iostream>
#include <fstream>
#include <string>
#include "ns3/amliran.h"
namespace ns3 {

class AmliranTracer
{
public:
	AmliranTracer();
	~AmliranTracer();
	void OpenTraceFile(std::string filename);
	void OpenTraceFile1(std::string filename);
	void CloseTraceFile();
	void streamSendRateTrace(uint16_t ssrc, float rate);
	void schedRateTrace(float rate);
	void recRateTrace(float rate);
	void streamRateTrace(uint16_t ssrc, float rate);
	void DelayTrace(double delay);
private:
	std::fstream m_streamSendRateFile[numStream];
	std::fstream m_schedRateFile;//
	std::fstream m_recRateFile;
	std::fstream m_traceDelayFile;
    std::fstream m_streamRateTraceFile[numStream];

};

}

#endif /* AMLIRANTRACE_H */

