#include "amliranRx.h"
//#include "screamTx.h"
#ifdef _MSC_VER
#define NOMINMAX
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif
#include <string.h>
#include <climits>
#include <algorithm>
#include <iostream>
using namespace std;

namespace ns3
{
AmliranRx::Stream::Stream(uint16_t ssrc_) {
    ssrc = ssrc_;
    bytesReceived = 0;
    lastRateComputeT_us = 0;
    averageReceivedRate = 0;
}

AmliranRx::AmliranRx(uint32_t ssrc_) {
    m_rateCalQueue = new RateCalQueue();
    m_tbCalQueue = new TbCalQueue();
    ssrc = ssrc_;
    bytesReceived = 0;
    lastRateComputeT_us = 0;
    averageReceivedRate = 0;
    sumPRec = 0;
    sumPSched = 0;
}

AmliranRx::~AmliranRx() {
    if (!streams.empty()) {
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            delete (*it); 
        }
    }
}
AmliranRx::Stream* AmliranRx::getStream(uint16_t ssrc) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if ((*it)->isMatch(ssrc)) {
            return (*it);
        }
    }
    return NULL;
}

uint16_t AmliranRx::upfReceive(int64_t time_us,
    void* rtpPacket,
    uint16_t ssrc,
    uint64_t seqNr,
    uint32_t size,
    uint32_t txTs,
    RateCalQueue* rateCalQueue
    //TbCalQueue* tbCalqueue
    ) {//到达UPF速率计算//////////////////////////
    uint16_t mark;
    //cout<<"------------------upfReceive: "<<time_us<<endl;
    if (!streams.empty()) {
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            if ((*it)->isMatch(ssrc)) {
                /*
                * Packets for this SSRC received earlier
                * stream is thus already in list
                */
                incToken[ssrc] = incToken[ssrc] - m_decFraction[ssrc] + m_incFraction[ssrc];
                if(incToken[ssrc] >= 1.0) {
                    incToken[ssrc] -= 1;
                    mark = 1;//increase mark
                }else if(incToken[ssrc] <= -1.0) {
                    incToken[ssrc] += 1;
                    mark = 2;//decrease mark
                }else {
                    mark = 3;// no action mark
                }
                rateCalQueue->push(rtpPacket,size,ssrc,seqNr,mark,(int64_t)txTs,time_us);
                return mark;
            }
        }
    }
    /*
    * New {SSRC,PT}
    */
    //cout<<"Need to generate a new stream!!!!!!!!"<<ssrc<<endl;
    Stream *stream = new Stream(ssrc);
    //stream->ix = ix++;
    //stream->receive(time_us, rtpPacket, size, seqNr);
    stream->strPriority = streamPriority[ssrc];
    //cout<<"stream "<<ssrc<<" priority is "<<stream->strPriority<<endl;
    stream->ssrc = ssrc;
    incToken[ssrc] = incToken[ssrc] - m_decFraction[ssrc] + m_incFraction[ssrc];
    if(incToken[ssrc] >= 1.0) {
        incToken[ssrc] -= 1;
        mark = 1;//increase mark
    }else if(incToken[ssrc] <= -1.0) {
        incToken[ssrc] += 1;
        mark = 2;//decrease mark
    }else {
        mark = 3;// no action mark
    }
    rateCalQueue->push(rtpPacket,size,ssrc,seqNr,mark,time_us,time_us);
    streams.push_back(stream);
   // cout<<"rate queue push successfully!!"<<ssrc<<mark<<endl;
    return mark;
}

void AmliranRx::calMarkFraction(int64_t time_us,
    AmlRlcQueue* rlcQueue,
    RateCalQueue* rateCalQueue,
    TbCalQueue* tbSchedQueue
) {
    rateCalQueue->calculateRate(time_us);
    tbSchedQueue->calSchedRate(time_us);
    //cout<<"cal Mark Fraction at "<<time_us<<endl;
    //cout<<"sched Rate: "<<tbSchedQueue->m_schedRate<<" and enqueue Rate: "<<rateCalQueue->m_rate<<" "<<rateCalQueue->m_streamRate[0]<<" "<<rateCalQueue->m_streamRate[1]<<endl;
    if(!m_schedRateCb.IsNull())
	{
	double mbps=tbSchedQueue->m_schedRate/1000000;
    //cout<<"sched Rate log file at "<<time_us<<"for Mbps "<<mbps<<endl;
	m_schedRateCb((float)mbps);
	}
    m_qLenTh = (tbSchedQueue->m_schedRate/8) * qDelayTh*1e-6;
    cout<<"m_qLenTh is "<<m_qLenTh<<" and bytes in rlcQueue is "<<rlcQueue->bytesInQueue()<<endl;
    if(rateCalQueue->m_rate > tbSchedQueue->m_schedRate) {
        double sumPriorityInv = 0;
        for(auto it = streams.begin(); it != streams.end(); ++it) {
            sumPriorityInv += 1.0/(*it)->strPriority;
        }
        double basicDecFraction = (rateCalQueue->m_rate - tbSchedQueue->m_schedRate) / rateCalQueue->m_rate;
        double numRTTwoviolate = (m_qLenTh - rlcQueue->bytesInQueue()) * 8 / ((rateCalQueue->m_rate - tbSchedQueue->m_schedRate)* RTT*1e-3);
        numRTTwoviolate = std::max(numRTTwoviolate,1.0);
        double sumDecFraction = basicDecFraction / numRTTwoviolate;

        for(auto it = streams.begin(); it != streams.end(); ++it) {
            if(m_qLenTh > rlcQueue->bytesInQueue()) {
                m_decFraction[(*it)->ssrc] = rateCalQueue->m_rate *sumDecFraction / ((*it)->strPriority * sumPriorityInv * rateCalQueue->m_streamRate[(*it)->ssrc]);
                cout<<"m_decFraction["<<(*it)->ssrc<<"]: "<<m_decFraction[(*it)->ssrc]<<endl;
                m_decFraction[(*it)->ssrc] = std::max(m_decFraction[(*it)->ssrc],0.13);
            }else {
                m_decFraction[(*it)->ssrc] = 0.13;
            }
            m_incFraction[(*it)->ssrc] = 0;
        }
    }else if(rateCalQueue->m_rate < tbSchedQueue->m_schedRate) {
        double sumPriority = 0;
        for(auto it = streams.begin(); it != streams.end(); ++it) {
            sumPriority += (*it)->strPriority;
        }
        double basicIncFraction = (tbSchedQueue->m_schedRate - rateCalQueue->m_rate) / rateCalQueue->m_rate;
        double numRTTwoviolate = rlcQueue->bytesInQueue() * 8 / ((tbSchedQueue->m_schedRate - rateCalQueue->m_rate)* RTT*1e-3);
        numRTTwoviolate = std::max(numRTTwoviolate,1.0);
        double sumIncFraction = basicIncFraction / numRTTwoviolate;
        
        for(auto it = streams.begin(); it != streams.end(); ++it) {
            if(rlcQueue->bytesInQueue() > 0) {
                m_incFraction[(*it)->ssrc] = (*it)->strPriority * rateCalQueue->m_rate *sumIncFraction / ( sumPriority * rateCalQueue->m_streamRate[(*it)->ssrc]);
                cout<<"m_incFraction["<<(*it)->ssrc<<"]: "<<m_incFraction[(*it)->ssrc]<<endl;
                m_incFraction[(*it)->ssrc] = std::min(std::sqrt(m_incFraction[(*it)->ssrc] * m_incFraction[(*it)->ssrc] * m_incFraction[(*it)->ssrc]),0.1);
            }else {
                m_incFraction[(*it)->ssrc] = 0.1;
            }
            m_decFraction[(*it)->ssrc] = 0;
        }
    }else {
        cout<<"m_incFraction and m_decFraction is 0"<<endl;
        for(auto it = streams.begin(); it != streams.end(); ++it) {
            m_incFraction[(*it)->ssrc] = 0;
            m_decFraction[(*it)->ssrc] = 0;
        }
    }
    //cout<<"have caled Mark Fraction"<<endl;
}

int64_t AmliranRx::isOkToSched(int64_t time_us,
    uint16_t& numPDeliver,
    FILE* tbSchedFp,
    AmlRlcQueue* rlcQueue,
    RateCalQueue* rateCalQueue,
    TbCalQueue* tbSchedQueue) {
    //cout<<"isOkToSched: "<<time_us<<endl;
    if(hasUnSchedTb) {
        if(time_us >=m_unSchedTbTs) {
            numPDeliver=0;
            while (m_unSchedTbSize > -pacSize/2){
                numPDeliver++;
                m_unSchedTbSize -=pacSize;
            }
            //cout<<"numPDeliver: "<<numPDeliver<<endl;
            tbSchedQueue->push(time_us,numPDeliver*pacSize);
            //cout<<"tbSchedQueue->push at "<<time_us<<"for byte "<<numPDeliver*pacSize<<endl;
            calMarkFraction(time_us,rlcQueue,rateCalQueue,tbSchedQueue);
            //cout<<"success, return: "<<endl;
            hasUnSchedTb=false;
            // no need to read Tb file again.
            return 0;
        }else {
            return m_unSchedTbTs - time_us;
        }
    }else {
        int32_t schedTime=0;int32_t schedSize=0;
        //cout<<"Need to read new TB from file at time: "<<time_us<<endl;
        if(fscanf(tbSchedFp,"%d %d",&schedTime,&schedSize)) {
            //cout<<"read TB successfully: "<<(int64_t)schedTime<<" "<<schedSize<<endl;
            while(schedTime< time_us - 500){
                fscanf(tbSchedFp,"%d %d",&schedTime,&schedSize);
                //cout<<"read TB successfully: "<<(int64_t)schedTime<<" "<<schedSize<<endl;
            }
            m_unSchedTbSize+=schedSize;
            if(time_us >= (int64_t)schedTime) {
                numPDeliver=0;
                while (m_unSchedTbSize > -pacSize/2){
                    numPDeliver++;
                    m_unSchedTbSize -=pacSize;
                }
                //cout<<"numPDeliver: "<<numPDeliver<<endl;
                tbSchedQueue->push(time_us,numPDeliver*pacSize);
                calMarkFraction(time_us,rlcQueue,rateCalQueue,tbSchedQueue);
                //cout<<"isOkToSched success, returning 0"<<endl;
                return 0;
            }else{
                hasUnSchedTb =true;
                m_unSchedTbTs = (int64_t)schedTime;
                //cout<<"isOkToSched success, returning "<<m_unSchedTbTs - time_us<<endl;
                return m_unSchedTbTs - time_us;
            }
        }
    }
}

//pop packet from RLC buffer, count the incPac...
bool AmliranRx::getSchedule(int64_t time_us,
    uint16_t numPDeliver,
    AmlRlcQueue* rlcQueue,
    uint64_t* seqNr,
    int16_t* incPac,
    int64_t* txTmsp) {

    void *rtpPacket=0;
    int size;
    if(rlcQueue->bytesInQueue() == 0){return false;}
    sumPSched += numPDeliver;
    //cout<<"numPDeliver: "<<numPDeliver<<endl;
    for(uint16_t i=0;i<numPDeliver; i++) {
        uint16_t ssrc;
        //uint64_t seqNr;
        uint16_t mark;
        int64_t enqueueTmsp;
        if(rlcQueue->bytesInQueue()>0){
            //cout<<"rlcQueue->bytesInQueue: "<<rlcQueue->bytesInQueue()<<endl;
            if(rlcQueue->deliverPacket(rtpPacket,size,ssrc,seqNr,txTmsp,enqueueTmsp,mark)) {
                //cout<<"rlcQueue->bytesInQueue after deliver: "<<rlcQueue->bytesInQueue()<<endl;
                //cout<<"mark: "<<mark<<endl;
                if(mark==1){incPac[ssrc] += 1;}
                else if(mark==2){incPac[ssrc]-=1;}
                //incPac[ssrc] += 1*(mark==1)-1*(mark==2);
                if(!m_delayCb.IsNull()){m_delayCb((time_us-enqueueTmsp)*1e-3);}

                //caculate receive rate and lof file
                bytesReceived += pacSize;
                if(time_us >= lastRateComputeT_us + rateWindow) {
                    //cout<<"Time: "<<time_us<<" bytesReceived: "<<bytesReceived<<". Time last: "<<time_us - lastRateComputeT_us<<endl;
                    averageReceivedRate = bytesReceived*8 / (time_us - lastRateComputeT_us);
                    lastRateComputeT_us = time_us;
                    bytesReceived = 0;
                    if(!m_recRateCb.IsNull()){m_recRateCb(averageReceivedRate);}
                }
                //calculate sum received data to get TB utilization
                sumPRec += 1;
                //caculate stream receive rate and lof file
                Stream* stream = getStream(ssrc);
                stream->bytesReceived += pacSize;
                if(stream->lastRateComputeT_us + rateWindow <= time_us) {
                    stream->averageReceivedRate = stream->bytesReceived*8 / 
                        (time_us - stream->lastRateComputeT_us);
                    stream->lastRateComputeT_us = time_us;
                    stream->bytesReceived = 0;
                    if(!m_streamRateCb.IsNull()){m_streamRateCb(ssrc,stream->averageReceivedRate);}
                }
            }
            
        }
        
    }
    if(time_us > 9890000){cout<<"TB utilization: "<<(double)sumPRec / (double)sumPSched<<endl;}
    //cout<<"incPac[0]: "<<incPac[0]<<" incPac[1]: "<<incPac[1]<<endl;
    return true;
}
int64_t AmliranRx::addDelivered(int64_t time_us,FILE* tbSchedFp) {
    int32_t schedTime=0;int32_t schedSize=0;
    if(fscanf(tbSchedFp,"%d %d",&schedTime, &schedSize)) {
        while(time_us > (int64_t)schedTime) {
            fscanf(tbSchedFp,"%d %d\n",&schedTime, &schedSize);
        }
        m_unSchedTbSize+=schedSize;
        hasUnSchedTb =true;
        m_unSchedTbTs = (int64_t)schedTime;
        return m_unSchedTbTs - time_us;
    }else {
        return -1;
    }
}

}
