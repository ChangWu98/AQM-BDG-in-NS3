#ifndef AMLIRAN_RX
#define AMLIRAN_RX
#include <cstdint>
#include <list>
#include "amlrlcqueue.h"

namespace ns3
{
class AmliranRx {
public:
    AmliranRx(uint32_t ssrc); // SSRC of this RTCP session
    ~AmliranRx();
    
    /*
    * One instance is created for each source SSRC
    */
    class Stream {
    public:
        Stream(uint16_t ssrc);
        bool isMatch(uint16_t ssrc_) { return ssrc == ssrc_; };
        uint16_t ssrc;                // SSRC of stream (source SSRC)
        double strPriority;
        int bytesReceived;
        int64_t lastRateComputeT_us;
        float averageReceivedRate;//Mbps
    };

    uint16_t ssrc;
    int bytesReceived;
    uint64_t sumPRec;
    uint64_t sumPSched;
    int64_t lastRateComputeT_us;
    float averageReceivedRate;//Mbps
    /*
    * Variables for multiple steams handling
    */
    std::list<Stream*> streams;
    Stream* getStream(uint16_t ssrc);

    /*
    * Function is called each time an RTP packet is received to RLC buffer
    */
    uint16_t upfReceive(int64_t time_us,
        void* rtpPacket,
        uint16_t ssrc,
        uint64_t seqNr,
        uint32_t size,
        uint32_t txTs,
        RateCalQueue* rateCalQueue
        //TbCalQueue* tbCalqueue
    );

    //uint16_t m_mark;
    bool hasUnSchedTb{false};
    int64_t m_unSchedTbTs;
    int32_t m_unSchedTbSize{0};
    int32_t m_oweSchedTbsize{0};
    //double m_incFraction{0};
    //double m_decFraction{0};
    double m_incFraction[numStream]={0.0};
    double m_decFraction[numStream]={0.0};
    double incToken[numStream]={0.0};
    uint32_t m_qLenTh{0};//byte
    void calMarkFraction(int64_t time_us,
        AmlRlcQueue* rlcQueue,
        RateCalQueue* rateCalQueue,
        TbCalQueue* tbSchedQueue
    );
    uint16_t ifMark(uint16_t ssrc,uint64_t seqNr,uint64_t time_us);

    /*
    * Function determines if and how many packets in RLC buffer can be delivered
    * Return values :
    * 0  : packets in RLC buffer can be immediately delvered. TB being schedled will be put into tbCalQueue.
    * getSchedule must be called imediately
    * >0 : Time [us] until this function should be called again. This can be used to start a timer
    * -1 : No packet available to deliver
    */
    int64_t isOkToSched(int64_t time_us,
        uint16_t& numPDeliver,
        FILE* tbSchedFp,
        AmlRlcQueue* rlcQueue,
        RateCalQueue* rateCalQueue,
        TbCalQueue* tbSchedQueue
    );
    int64_t addDelivered(int64_t time_us, FILE* tbSchedFp);
    //get ssrc and seqNr fedback to acquire send time
    bool getSchedule(int64_t time_us,
        uint16_t numPDeliver,
        AmlRlcQueue* rlcQueue,
        uint64_t* seqNr,
        int16_t* incPac,
        int64_t* tmsp);
    RateCalQueue *m_rateCalQueue; 
    TbCalQueue *m_tbCalQueue;


    typedef Callback<void,double> DelayCallback;
	void SetDelayCallback(DelayCallback cb){m_delayCb=cb;}

    typedef Callback<void,float> schedRateCallback;
    void SetSchedRateCallback(schedRateCallback cb){m_schedRateCb=cb;}

    typedef Callback<void,float> recRateCallback;
    void SetRecRateCallback(recRateCallback cb){m_recRateCb=cb;}

    typedef Callback<void,uint16_t,float> streamRateCallback;
    void SetStreamRateCallback(streamRateCallback cb){m_streamRateCb=cb;}

private:
    DelayCallback m_delayCb;//ms
    schedRateCallback m_schedRateCb;//Mbps
    recRateCallback m_recRateCb;//Mbps
    streamRateCallback m_streamRateCb;//Mbps
};
}
#endif
