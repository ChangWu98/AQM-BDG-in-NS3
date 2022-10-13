#include "amliranTx.h"
#include "amliranRx.h"
#ifdef _MSC_VER
#define NOMINMAX
#include <winSock2.h>
#else
#include <arpa/inet.h>
#endif
#include <cstdint>
#include <cmath>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ns3/log.h"
using namespace std;
namespace ns3{
NS_LOG_COMPONENT_DEFINE("AmliranTx");
AmliranTx::AmliranTx(/* args */) {
    nStreams = 0;
    for (int n = 0; n < kMaxStreams; n++)
        streams[n] = NULL;
}
    
AmliranTx::~AmliranTx() {
    for (int n = 0; n < nStreams; n++)
        delete streams[n];
} 

void AmliranTx::registerNewStream(uint16_t ssrc,
            float minBitrate,
            float startBitrate,
            float maxBitrate) {
    Stream *stream = new Stream(this,ssrc,minBitrate,startBitrate,maxBitrate);
    m_tracer.OpenTraceFile1(string("amliran/"));
    stream->SetStreamSendRateCallback(MakeCallback(&AmliranTracer::streamSendRateTrace,&m_tracer));
    streams[nStreams++] = stream;
}
int32_t AmliranTx::isOkToTransmit(int64_t time_us, uint16_t& ssrc) {
    //cout<<"isOkToTransmit: "<<time_us<<endl;
    int32_t ret = 10000;
    for(int i=0;i<numStream;i++) {
        int streamId;
        Stream* stream = getStream(i,streamId);
        int32_t sendIntv_us=1000000.0 / (stream->getTargetBitrate(time_us)/(8 * pacSize));
        //cout<<"numStream "<<i<<" "<<"send Interval: "<<sendIntv_us<<endl;
        //cout<<"Stream "<<i<<" last send Time: "<<stream->lastSendT_us<<endl;
        if(time_us >= stream->lastSendT_us + sendIntv_us){
            ssrc = i;
            //cout<<"stream "<<ssrc<<" get last send Time "<<stream->lastSendT_us<<endl;
            stream->lastSendT_us = time_us;
            return 0;
        }else {
            ret = std::min((int64_t)ret, stream->lastSendT_us + sendIntv_us - time_us);
        }
    }
    //cout<<"ret: "<<ret<<endl;
    return ret;
}
int32_t AmliranTx::addTransmitted(int64_t time_us,
    uint16_t &ssrc,
    int size,
    uint64_t seqNr) {
    int32_t ret = 10000;
    for(int i=0;i<numStream;i++) {
        int streamId;
        Stream* stream = getStream(i,streamId);
        int32_t sendIntv_us=1000000.0 / (stream->getTargetBitrate(time_us)/(8 * pacSize));
        if(time_us >= stream->lastSendT_us + sendIntv_us){
            ssrc = i;
            return 0;
        }else {
            ret = std::min((int64_t)ret, stream->lastSendT_us + sendIntv_us - time_us);
        }
    }
    return ret;
        
}
void AmliranTx::incomingFeedback(int64_t time_us,
    int64_t* rxTs,
    uint64_t* seqNr,
    int16_t* incPac) {
        for(int i=0;i<numStream;i++) {
            if(rxTs[i] != 0) {
                int streamId;
                Stream* stream = getStream(i,streamId);
                stream->receiveFb(rxTs[i],seqNr[i],incPac[i]);
            }
        }

}
AmliranTx::Stream* AmliranTx::getStream(uint16_t ssrc, int &streamId) {
    for (int n = 0; n < nStreams; n++) {
        if (streams[n]->isMatch(ssrc)) {
            streamId = n;
            return streams[n];
        }
    }
    streamId = -1;
    return NULL;
}


AmliranTx::Stream::Stream(AmliranTx *parent_,
    uint16_t ssrc_,
    float minBitrate_,
    float startBitrate_,
    float maxBitrate_) {
    parent = parent_;
    ssrc = ssrc_;
    minBitrate = minBitrate_;
    maxBitrate = maxBitrate_;
    targetBitrate = std::min(maxBitrate, std::max(minBitrate, startBitrate_));
    int64_t now_us =Simulator::Now().GetMicroSeconds();
    lastSendT_us = 0;
    lastRateAdjustT_us = now_us;
    hSendRateQueue.push(now_us,targetBitrate);
}

void AmliranTx::Stream::updateRate(int64_t time_us) {
    //if(time_us < 360000){return;}
    float sendRateNewest = hSendRateQueue.getSendRate() + getChangeOfRate(time_us);
    //cout<<"ChangeOfRate at "<<time_us<<" is "<<getChangeOfRate(time_us)<<endl;
    //cout<<"old send rate at "<<time_us<<" is "<<hSendRateQueue.getSendRate()<<"   sendRateNewest is "<<sendRateNewest<<endl;
    if(!m_streamSendRateCb.IsNull()){m_streamSendRateCb(ssrc,sendRateNewest/1000000);}
    sendRateNewest = std::min(maxBitrate, std::max(minBitrate, sendRateNewest));
    //sendRateNewest = 12500000;
    
    hSendRateQueue.push(time_us,sendRateNewest);
    lastRateAdjustT_us = time_us;
}

float AmliranTx::Stream::getTargetBitrate(int64_t time_us) {
    //cout<<"getTargetBitrate: "<<time_us<<"lastRateAdjustT_us: "<<lastRateAdjustT_us<<endl;
    if(time_us - lastRateAdjustT_us > sendRateUpdateInterval){
        //cout<<"update Rate: "<<time_us<<endl;
        updateRate(time_us);
        firstRun = false;
    }
    return hSendRateQueue.getSendRate();
}

float AmliranTx::Stream::getChangeOfRate(int64_t time_us) {
    fbInfoQueue.updateInfoQueue();
    hSendRateQueue.updateRateQueue(fbInfoQueue.getTailTs());

    float changeOfRate_t = 0;
    static int lowFbInfoPtr = 1;
    lowFbInfoPtr = fbInfoQueue.tail;
    float numChange = 0;
    int highestFbInfoPtr = fbInfoQueue.tail;
    //cout<<"fbInfoQueue.tail: "<<fbInfoQueue.tail<<endl;
    cout<<"hSendRateQueue has "<<hSendRateQueue.nItems<<" items and fbInfoQueue has "<<fbInfoQueue.nItems<<" items"<<endl;
    for(int i=0;i<hSendRateQueue.nItems;i++)
    {
        //cout<<"hSendRateQueue.items["<<(hSendRateQueue.tail+i)%queueSize<<"]->ts: "<<hSendRateQueue.items[(hSendRateQueue.tail+i)%queueSize]->ts<<endl;
        uint64_t seqNrN = 0;
        int16_t incPacN = 0;
        float sendRateL = hSendRateQueue.items[(hSendRateQueue.tail+i)%queueSize]->sendRate;
        int64_t sendRateTsL = hSendRateQueue.items[(hSendRateQueue.tail+i)%queueSize]->ts;
        int lowestFbInfoPtr = fbInfoQueue.tail;
        
        if(fbInfoQueue.nItems < 2){return 0.0;}
        if((hSendRateQueue.tail+i)%queueSize == hSendRateQueue.head)
        {
            cout<<"The last send Rate in the Queue!"<<endl;
            seqNrN = fbInfoQueue.items[fbInfoQueue.head]->seqNr - 
                fbInfoQueue.items[highestFbInfoPtr]->seqNr;
            for(int j=0; j<fbInfoQueue.nItems; j++) {
                if(fbInfoQueue.items[(fbInfoQueue.tail+j)%queueSize]->timestamp >= sendRateTsL)
                    incPacN += fbInfoQueue.items[(fbInfoQueue.tail+j)%queueSize]->incPac;
            }
        }else {
            for(int j=0; j<fbInfoQueue.nItems; j++) {
                if(fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->timestamp >= 
                    hSendRateQueue.items[(hSendRateQueue.tail+i)%queueSize]->ts &&
                    fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->timestamp <
                    hSendRateQueue.items[(hSendRateQueue.tail+i+1)%queueSize]->ts) 
                    {
                    //cout<<"fbInfoQueue.items["<<(lowestFbInfoPtr+j)%queueSize<<"]->incPac: "<<fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->incPac<<endl;
                    cout<<"fbInfoQueue.items["<<(lowestFbInfoPtr+j)%queueSize<<"]->timestamp: "<<fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->timestamp<<endl;
                    //cout<<"hSendRateQueue.items["<<(hSendRateQueue.tail+i+1)%queueSize<<"]->ts: "<<hSendRateQueue.items[(hSendRateQueue.tail+i+1)%queueSize]->ts<<endl;
                    incPacN += fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->incPac;
                }
                if(fbInfoQueue.items[(lowestFbInfoPtr+j)%queueSize]->timestamp < 
                    hSendRateQueue.items[(hSendRateQueue.tail+i+1)%queueSize]->ts)
                    {
                    highestFbInfoPtr = (fbInfoQueue.tail+j)%queueSize;

                }
            }
            cout<<"highestFbInfoPtr: "<<highestFbInfoPtr<<endl;
            seqNrN = fbInfoQueue.items[highestFbInfoPtr]->seqNr - fbInfoQueue.items[lowFbInfoPtr]->seqNr;
            cout<<"lowFbInfoPtr: "<<lowFbInfoPtr<<endl;
            lowFbInfoPtr = highestFbInfoPtr;
        }
        cout<<"incPacN is "<<incPacN<<" and seqNrN is "<<seqNrN<<endl;
        if(seqNrN >= 5){
            changeOfRate_t += sendRateL * (incPacN)/(seqNrN+1);
            numChange += 1;
        }
        
    }
    return changeOfRate_t / numChange;    
}
void AmliranTx::Stream::receiveFb(int64_t rxTs,uint64_t seqNr,int16_t incPac) {
    fbInfoQueue.push(rxTs,seqNr,incPac);
}



}