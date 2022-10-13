#ifndef AML_RLC_QUEUE
#define AML_RLC_QUEUE
#include "amliran.h"

/*
* Implements a simple RTP packet queue, one RTP queue 
* per stream {SSRC,PT}
*/
namespace ns3
{
class AmlRlcQueueIface {
public:
    virtual void clear() = 0;
    virtual int sizeOfNextPac() = 0;
    virtual uint64_t seqNrOfNextPac() = 0;
    virtual uint32_t bytesInQueue() = 0; // Number of bytes in queue
    virtual uint32_t sizeOfQueue() = 0;  // Number of items in queue
    virtual int64_t getDelay(int64_t currTs) = 0;
virtual ~AmlRlcQueueIface(){}
};

class AmlRlcQueueItem {
public:
    AmlRlcQueueItem();
    void* packet;
    int size;
    uint64_t seqNr;
    int64_t txTs;
    int64_t enqueueTs;
    bool used;
    int ssrc;
    int mark;
};

const int queueSize = 2000000;

class AmlRlcQueue : public AmlRlcQueueIface {
public:
    AmlRlcQueue();
    void push(void *rtpPacket, int size, uint16_t ssrc, uint64_t seqNr, uint16_t mark, int64_t ts,int64_t enqueueTs);
    bool pop(void *rtpPacket, int &size, uint16_t& ssrc,uint64_t* seqNr,int64_t* tmsp, int64_t& enqueueTmsp,uint16_t& mark);
    int sizeOfNextPac();
    uint64_t seqNrOfNextPac();
    uint32_t bytesInQueue(); // Number of bytes in queue
    uint32_t bytesInQueue(uint16_t ssrc);
    uint32_t sizeOfQueue();  // Number of items in queue
    int64_t getDelay(int64_t time_us);
    int64_t getDelay(int64_t time_us, uint16_t ssrc);
    bool deliverPacket(void *rtpPacket,int &size,uint16_t& ssrc,uint64_t* seqNr, int64_t* tmsp,int64_t& euqueueTmsp, uint16_t& mark);
    void clear();
    void computeSizeOfNextRtp();

    AmlRlcQueueItem *items[queueSize];
    int head; // Pointer to last inserted item
    int tail; // Pointer to the oldest item
    int nItems;

    int bytesInQueue_;
    int bytesInStreamQueue_[numStream];
    int sizeOfQueue_;
    int sizeOfNextPac_;
};

class RateCalQueue : public AmlRlcQueue {
    public:
    RateCalQueue();
    void calculateRate(int64_t time_us);
    bool adjustRateQueue(int64_t time_us);
    double m_rate;//bps
    double m_streamRate[numStream];//bps
    bool pop();
};

class TbSchedQueueItem
{
public:
    TbSchedQueueItem(/* args */);
    int64_t ts;
    bool used{false};
    int32_t tbSize{0};//Byte
};
class TbCalQueue {
    public:
    TbCalQueue();
    void push(int64_t ts, int32_t tbSize);
    bool pop(int32_t& tbSize);
    int32_t bytesInQueue(); // Number of bytes in queue
    int64_t getDelay(int64_t time_us);
    void clear();
    void computeSizeOfLastTb();
    void calSchedRate(int64_t time_us);
    bool adjustSchedQueue(int64_t time_us);

    TbSchedQueueItem *items[queueSize];
    int head; // Pointer to last inserted item
    int tail; // Pointer to the oldest item
    int nItems;
    int bytesInQueue_;
    int sizeOfLastTb_;
    double m_schedRate;//bps
};

//history send rate queue
class HSendRateQueueItem
{
public:
    HSendRateQueueItem();
    int64_t ts;
    float sendRate;//bps
};
class HSendRateQueue {
public:
    HSendRateQueue();
    void push(int64_t time_us, float sendRate);
    bool pop();
    bool updateRateQueue(int64_t time_us);
    float getSendRate();
    float getHistRate(int64_t time_us);

    HSendRateQueueItem *items[queueSize];
    int head; // Pointer to last inserted item
    int tail; // Pointer to the oldest item
    int nItems;
    float rate;
};

class FbInfoQueueItem
{
public:
    FbInfoQueueItem();
    int64_t timestamp;//timestamp of first packet within m_numPDeliver packets
    uint64_t seqNr;//lowest sequence number of packet has received of every stream
    int16_t incPac;//number of packets with "increase", negtive with "decrease"
};
class FbInfoQueue
{
public:
    FbInfoQueue();
    void push(int64_t ts,uint64_t seqNr,int16_t incPac);
    bool pop();
    void updateInfoQueue();
    //float calculateFrac();
    int64_t getTailTs();
    
    FbInfoQueueItem *items[queueSize];
    int head; // Pointer to last inserted item
    int tail; // Pointer to the oldest item
    int nItems;
};

	
}

#endif

