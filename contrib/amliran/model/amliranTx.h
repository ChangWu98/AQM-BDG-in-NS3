#ifndef AMLIRANTX_H
#define AMLIRANTX_H
#include <string.h>
#include <iostream>
#include <cstdint>
#include "amlrlcqueue.h"
#include "ns3/amlirantrace-module.h"

namespace ns3{
const int kMaxStreams = 20;
    class AmliranTx
    {
    public:
        AmliranTx(/* args */);
        ~AmliranTx();
        void registerNewStream(uint16_t ssrc,
            float minBitrate,
            float startBitrate,
            float maxBitrate);
        int32_t isOkToTransmit(int64_t time_us, uint16_t& ssrc);
        int32_t addTransmitted(int64_t time_us, uint16_t& ssrc, int size, uint64_t seqNr);
        void incomingFeedback(int64_t time_us, int64_t* txTs, uint64_t* seqNr, int16_t* incPac);
        float getTargetBitrate(uint16_t ssrc);


    private:
        class Stream
        {
        public:
            Stream(AmliranTx *parent,
                uint16_t ssrc,
                float minBitrate,
                float startBitrate,
                float maxBitrate
            );
            AmliranTx *parent;
            uint16_t ssrc;          // SSRC of stream
            float minBitrate;       // Min bitrate
            float maxBitrate;       // Max bitrate
            float targetBitrate;    // Target bitrate
            int64_t lastSendT_us;
            int64_t lastRateAdjustT_us; // Last time rate was updated for this stream
            void updateRate(int64_t time_us);
            float getTargetBitrate(int64_t time_us);
            bool isMatch(uint16_t ssrc_) { return ssrc == ssrc_; };
            bool firstRun{true};
            HSendRateQueue hSendRateQueue;//history send rate queue
            FbInfoQueue fbInfoQueue;
            float getChangeOfRate(int64_t time_us);
            void receiveFb(int64_t rxTs,uint64_t seqNr,int16_t incPac);

            typedef Callback<void,uint16_t,float> streamSendRateCallback;
            void SetStreamSendRateCallback(streamSendRateCallback cb){m_streamSendRateCb=cb;}
            
            private:
            streamSendRateCallback m_streamSendRateCb;//Mbps
        };
        /* data */
        Stream* streams[kMaxStreams];
        int nStreams;
        /*
        * The the stream that matches SSRC
        */
        Stream* getStream(uint16_t ssrc, int &streamId);

        AmliranTracer m_tracer;
    };


    
}
#endif