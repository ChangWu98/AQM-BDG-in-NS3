#ifndef AMLIRAN_HEADER_H
#define AMLIRAN_HEADER_H
#include "ns3/header.h"
#include "ns3/type-id.h"
#include "ns3/amliran.h"
#include <iostream>
namespace  ns3
{
    //static const int numStream = 2;
    //version 0 mean rtp, 1 mean rtcp
    typedef struct 
    {
        /* data */
    #if 0
        uint16_t version:2;
        uint16_t padbit:1;
	    uint16_t extbit:1;
	    uint16_t cc:4;
	    uint16_t markbit:1;
	    uint16_t paytype:7;
    #else
        uint16_t cc:4;
	    uint16_t extbit:1;
	    uint16_t padbit:1;
        //uint16_t version:2;
	    uint16_t version:1;
	    uint16_t paytype:7;
        //uint16_t markbit:1;
	    uint16_t markbit:2;
    #endif
        uint64_t seq_number;
	    uint32_t timestamp;
	    uint16_t ssrc;
    }rtp_hdr_t;
    /*
     * RTCP commom header word
     */
    typedef struct{
    #if 0  //BIG_ENDIA
        uint16_t version:2;
        uint16_t padbit:1;
        uint16_t rc:5;
        uint16_t packet_type:8;
    #else
        uint16_t rc:5;
	    uint16_t padbit:1;
	    uint16_t version:2;
	    uint16_t packet_type:8;
    #endif
        uint16_t length;   /* pkt len in words, w/o this word */

    }rtcp_common_t;
    typedef struct{
	    int64_t timestamp[numStream];//highest timestamp of packet has received of every stream
        uint64_t seqNr[numStream];//highest sequence number of packet has received of every stream
        int16_t incPac[numStream];//number of packets with "increase", negtive with "decrease"
    }amliran_feedback_t;
    #define AMLIRAN_HDR_LEN 128
    enum AmliranProto
    {
        SC_RTP,
        SC_RTCP,  //for now,just this two
    };
    class AmliranHeader:public ns3::Header
    {
    public:
        AmliranHeader();
        ~AmliranHeader();
        void SetMid(uint32_t mid){m_mid=mid;}
        uint32_t SetHeaderPayload(void *buf,uint32_t buflen);
        uint32_t GetHeaderPayload(void *buf,uint32_t buflen);
        static ns3::TypeId GetTypeId();
        virtual ns3::TypeId GetInstanceTypeId(void)const;
        virtual uint32_t GetSerializedSize(void)const;
        virtual void Serialize(ns3::Buffer::Iterator start)const;
        virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
        virtual void Print(std::ostream &os)const;
    private:
        uint8_t m_mid; //the first serialized byte
        uint8_t m_hdr[AMLIRAN_HDR_LEN];//hdr
        uint32_t m_len;
    };
}
#endif