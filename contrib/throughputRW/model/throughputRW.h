/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef THROUGHPUTRW_H
#define THROUGHPUTRW_H
#include <string>
#include <unistd.h>
#include <fstream>
#include <iostream>
namespace ns3
{
#define MAXUSERNUM 40
class throughputRW
{
private:
    uint32_t m_userNum;
    long m_throughPut[8][MAXUSERNUM];
    uint32_t m_addNum; 
    float m_getRecordTime;
public:
    throughputRW();
    ~throughputRW();
    void thrwInitialize(uint32_t userNum);
    void addSingleRate(int sec,int ssrc,long rateData);
    void writeToFile();  
    bool isOkWriteToFile(int sec);
};

}

#endif /* THROUGHPUTRW_H */

