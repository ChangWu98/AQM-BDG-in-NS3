/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <unistd.h>
#include <memory.h>
#include <string>
#include <stdio.h>
#include <fstream>
#include "throughputRW.h"

using namespace std;
namespace ns3
{
throughputRW::throughputRW()
{
    m_userNum=0;
    m_addNum=0;
    m_getRecordTime=9.0;
}
throughputRW::~throughputRW()
{
    delete m_throughPut;
}
void throughputRW::thrwInitialize(uint32_t userNum)
{
    m_userNum=userNum;
    m_addNum=0;
}
void throughputRW::writeToFile(){
    char buf[FILENAME_MAX];
    string thoughpuPath=string (getcwd(buf, FILENAME_MAX)) + "/traces/" + "UeNum"+to_string(m_userNum)+"/thoughput.txt";
    ofstream fout(thoughpuPath);
    for(int i=0;i<8;i++){
        for(uint32_t j=0;j<m_userNum;j++){
            fout<<" "<<m_throughPut[i][j];
        }
        fout<<"\n";
    }
    fout.close();
}
void throughputRW::addSingleRate(int sec,int ssrc,long rateData)
{
    m_throughPut[sec-int(m_getRecordTime)][ssrc-1]=rateData;
    m_addNum++;
}
bool throughputRW::isOkWriteToFile(int sec){
    if(sec==int(m_getRecordTime+7) && m_addNum==8*m_userNum)
    return true;
    else
    return false;
}
}

