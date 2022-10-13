#include <unistd.h>
#include <memory.h>
#include <string>
#include<stdio.h>
#include "ns3/mytrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("My-Trace");
MyTracer::MyTracer(){}
MyTracer::~MyTracer()
{
	if(m_traceRateFile.is_open())
		m_traceRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();
	if(m_throughputFile.is_open())
		m_throughputFile.close();
}
void MyTracer::OpenTraceFile(std::string filename)
{
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	string delaypath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"delay.txt";
	memset(buf,0,FILENAME_MAX);
	string ratepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"sendrate.txt";
	memset(buf,0,FILENAME_MAX);
	string throughputpath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"recRate.txt";
	m_traceRateFile.open(ratepath.c_str(), fstream::out);
	m_traceDelayFile.open(delaypath.c_str(), fstream::out);
	m_throughputFile.open(throughputpath.c_str(), fstream::out);
}
void MyTracer::CloseTraceFile()
{
	if(m_traceRateFile.is_open())
		m_traceRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();
	if(m_throughputFile.is_open())
		m_throughputFile.close();	
}
void MyTracer::RateTrace(float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",	now,rate);
	m_traceRateFile<<line<<std::endl;		
}
void MyTracer::ThroughputTrace(float time_s,float rate)
{
	char line [255];
	memset(line,0,255);
	//double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",	time_s,rate);
	m_throughputFile<<line<<std::endl;		
}
void MyTracer::DelayTrace(float delay)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetMilliSeconds();
	//Time temp=MilliSenconds(delay);
	sprintf (line, "%16f %16f",	now*1e-3 - 10,delay);
	m_traceDelayFile<<line<<std::endl;
}
}
