/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <unistd.h>
#include <memory.h>
#include <string>
#include<stdio.h>
#include "amlirantrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("Amliran-Trace");
AmliranTracer::AmliranTracer(){}
AmliranTracer::~AmliranTracer()
{
	if(m_schedRateFile.is_open())
		m_schedRateFile.close();
	if(m_recRateFile.is_open())
		m_recRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();
	for(int i=0;i<numStream;i++) {
		if(m_streamRateTraceFile[i].is_open()){
			m_streamRateTraceFile[i].close();
		}
		if(m_streamSendRateFile[i].is_open()) {
			m_streamSendRateFile[i].close();
		}
	}
}
void AmliranTracer::OpenTraceFile(std::string filename)
{
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	string delaypath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"delay.txt";
	memset(buf,0,FILENAME_MAX);
	string schedRatepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"schedRate.txt";
	memset(buf,0,FILENAME_MAX);
	string recRatepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"recRate.txt";
	for(int i=0;i<numStream;i++) {
		memset(buf,0,FILENAME_MAX);
		string streamNr = i+"";
		string streamratepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"stream"+to_string(i)+"_rate.txt";
		m_streamRateTraceFile[i].open(streamratepath.c_str(), fstream::out);
	}
	m_schedRateFile.open(schedRatepath.c_str(), fstream::out);
	m_recRateFile.open(recRatepath.c_str(), fstream::out);
	m_traceDelayFile.open(delaypath.c_str(), fstream::out);
}
void AmliranTracer::OpenTraceFile1(std::string filename) {
	char buf[FILENAME_MAX];
	for(int i=0;i<numStream;i++) {
		memset(buf,0,FILENAME_MAX);
		string streamNr = i+"";
		string streamsendratepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"send"+to_string(i)+"_rate.txt";
		m_streamSendRateFile[i].open(streamsendratepath.c_str(), fstream::out);
	}
}
void AmliranTracer::CloseTraceFile()
{
	if(m_schedRateFile.is_open())
		m_schedRateFile.close();
	if(m_recRateFile.is_open())
		m_recRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();
	for(int i=0;i<numStream;i++) {
		if(m_streamRateTraceFile[i].is_open())
			m_streamRateTraceFile[i].close();
		if(m_streamSendRateFile[i].is_open())
			m_streamSendRateFile[i].close();
	}	
}

void AmliranTracer::streamSendRateTrace(uint16_t ssrc, float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	//cout<<"Time: "<<now<<" stream "<<ssrc<<" SendRate: "<<rate<<endl;
	sprintf (line, "%16f %16f",	now,rate);
	m_streamSendRateFile[ssrc]<<line<<std::endl;		
}

void AmliranTracer::schedRateTrace(float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",	now,rate);
	m_schedRateFile<<line<<std::endl;		
}
void AmliranTracer::recRateTrace(float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",	now,rate);
	m_recRateFile<<line<<std::endl;		
}
void AmliranTracer::streamRateTrace(uint16_t ssrc, float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",	now,rate);
	m_streamRateTraceFile[ssrc]<<line<<std::endl;		
}

void AmliranTracer::DelayTrace(double delay)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	//Time temp=MilliSenconds(delay);
	sprintf (line, "%16f %16f",	now,delay);
	m_traceDelayFile<<line<<std::endl;
}
}

