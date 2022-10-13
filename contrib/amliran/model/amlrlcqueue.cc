#include "amlrlcqueue.h"
#include <iostream>
#include <string.h>
using namespace std;
/*
* Implements a simple RTP packet queue
*/

namespace ns3
{
AmlRlcQueueItem::AmlRlcQueueItem() {
    packet = 0;
    used = false;
    size = 0;
    seqNr = 0;
}


AmlRlcQueue::AmlRlcQueue() {
    for (int n=0; n < queueSize; n++) {
        items[n] = new AmlRlcQueueItem();
    }
    head = -1;
    tail = 0;
    nItems = 0;
    bytesInQueue_ = 0;
    for(int i=0; i<numStream; i++){bytesInStreamQueue_[i]=0;}
    sizeOfQueue_ = 0;
    sizeOfNextPac_ = -1;
}

void AmlRlcQueue::push(void *rtpPacket,
    int size,
    uint16_t ssrc,
    uint64_t seqNr,
    uint16_t mark,
    int64_t Txts,
    int64_t enqueueTs) {
    head++; if (head == queueSize) {head = 0;}
    items[head]->packet = rtpPacket;
    items[head]->ssrc = ssrc;
    items[head]->seqNr = seqNr;
    items[head]->size = size;
    items[head]->txTs = Txts;
    items[head]->enqueueTs = enqueueTs;
    items[head]->used = true;
    items[head]->mark = mark;
    bytesInQueue_ += size;
    bytesInStreamQueue_[ssrc] += size;
    sizeOfQueue_ += 1;
    computeSizeOfNextRtp();
}

bool AmlRlcQueue::pop(void *rtpPacket,int& size,uint16_t& ssrc,uint64_t* seqNr, int64_t* tmsp,int64_t& enqueueTmsp, uint16_t& mark) {
    if (items[tail]->used == false) {
        sizeOfNextPac_ = -1;
        return false;
    } else {
        rtpPacket = items[tail]->packet;
        size = items[tail]->size;
        ssrc = items[tail]->ssrc;
        seqNr[items[tail]->ssrc] = items[tail]->seqNr;
        mark = items[tail]->mark;
        tmsp[items[tail]->ssrc] = items[tail]->txTs;
        enqueueTmsp = items[tail]->enqueueTs;
        items[tail]->used = false;
        bytesInStreamQueue_[items[tail]->ssrc] -= size;

        tail++; if (tail == queueSize) tail = 0;
        bytesInQueue_ -= size;
        sizeOfQueue_ -= 1;
        computeSizeOfNextRtp();
        return true;
    }
}

void AmlRlcQueue::computeSizeOfNextRtp() {
    if (!items[tail]->used) {
        sizeOfNextPac_ = - 1;
    } else {
        sizeOfNextPac_ = items[tail]->size;
    }
}

int AmlRlcQueue::sizeOfNextPac() {
    return sizeOfNextPac_;
}

uint64_t AmlRlcQueue::seqNrOfNextPac() {
    if (!items[tail]->used) {
        return -1;
    } else {
        return items[tail]->seqNr;
    }
}

uint32_t AmlRlcQueue::bytesInQueue() {
    return bytesInQueue_;
}

uint32_t AmlRlcQueue::bytesInQueue(uint16_t ssrc) {
    return bytesInStreamQueue_[ssrc];
}

uint32_t AmlRlcQueue::sizeOfQueue() {
    return sizeOfQueue_;
}

int64_t AmlRlcQueue::getDelay(int64_t time_us) {
    if (items[tail]->used == false) {
        return 0;
    } else {
        return time_us-items[tail]->txTs;
    }
}

int64_t AmlRlcQueue::getDelay(int64_t time_us, uint16_t ssrc) {
    int tailTemp = tail;
    while(items[tailTemp]->ssrc != ssrc) {tailTemp=(tailTemp+1)%queueSize;}
    return time_us-items[tailTemp]->txTs;
}

bool AmlRlcQueue::deliverPacket(void *rtpPacket,
    int& size,
    uint16_t& ssrc,
    uint64_t* seqNr,
    int64_t* txTmsp,
    int64_t& enqueueTmsp,
    uint16_t& mark) {
    if (sizeOfQueue() > 0) {
        //cout<<"size of amlRlcQueue: "<<sizeOfQueue_<<endl;
        pop(rtpPacket,size,ssrc,seqNr,txTmsp,enqueueTmsp, mark);
        //cout<<"size of amlRlcQueue after pop: "<<sizeOfQueue_<<endl;
        return true;
    }
    return false;
}

void AmlRlcQueue::clear() {
    for (int n=0; n < queueSize; n++) {
        items[n]->used = false;  
    }
    head = -1;
    tail = 0;
    nItems = 0;
    bytesInQueue_ = 0;
    sizeOfQueue_ = 0;
    sizeOfNextPac_ = -1;
}

// RateCalQueue methods
RateCalQueue::RateCalQueue() {
    m_rate = 0;
    double strStartRate = 0;
	if (numStream == 1){strStartRate = sumStartRate;}
	else {strStartRate = sumStartRate/3;}
    for(int i=0; i<numStream; i++) {
        m_streamRate[i] = (i+1)*strStartRate;
        //m_streamRate[i] = 15e6;
    }
    m_rate = sumStartRate;
}

void RateCalQueue::calculateRate(int64_t time_us) {  
    if(sizeOfQueue() < 8 ||time_us< 110000){return;}
    adjustRateQueue(time_us);
    if(sizeOfQueue_ != 0) {
         m_rate = bytesInQueue() *8 *1e6 / getDelay(time_us);
    }
    for(int i=0; i<numStream; i++) {
        if(bytesInStreamQueue_[i]>0) {
            m_streamRate[i] = bytesInStreamQueue_[i] *8 *1e6 / getDelay(time_us, i);
        }
            // else {
            //     m_streamRate[i] = 0;
            // }   
    }
    
    // else {
    //     m_rate = 0;
    //     for(int i=0; i<numStream; i++){
    //         m_streamRate[i] = 0;
    //     }
    // }
}

bool RateCalQueue::adjustRateQueue(int64_t time_us) {
    if(sizeOfQueue() == 0) {
        return false;
    } else {
        while(items[tail]->txTs < time_us-rateWindow) {pop();}
    }
}

bool RateCalQueue::pop() {
    if (items[tail]->used == false) {
        sizeOfNextPac_ = -1;
        return false;
    } else {
        items[tail]->used = false;
        bytesInQueue_ -= items[tail]->size;
        bytesInStreamQueue_[items[tail]->ssrc] -= items[tail]->size;

        tail++; if (tail == queueSize) tail = 0;
        sizeOfQueue_ -= 1;
        computeSizeOfNextRtp();
        return true;
    }
}

// TbSchedQueueItem methods
TbSchedQueueItem::TbSchedQueueItem() {
    tbSize = 0;
    used = false;
}
// TbCalQueue methods
TbCalQueue::TbCalQueue() {
    for (int n=0; n < queueSize; n++) {
        items[n] = new TbSchedQueueItem();
    }
    head = -1;
    tail = 0;
    nItems = 0;
    bytesInQueue_ = 0;
    sizeOfLastTb_ = -1;
    m_schedRate = 17e6;
}
void TbCalQueue::push(int64_t ts, int32_t tbSize) {
    //cout<<"tbSchedQueue->push at "<<ts<<endl;
    head++; if (head == queueSize) {head = 0;}
    items[head]->ts = ts;
    items[head]->used = true;
    items[head]->tbSize = tbSize;
    bytesInQueue_ += tbSize;
    computeSizeOfLastTb();
}
bool TbCalQueue::pop(int32_t& tbSize) {
    if (items[tail]->used == false) {
        sizeOfLastTb_ = -1;
        return false;
    } else {
        tbSize = items[tail]->tbSize;
        items[tail]->used = false;

        tail++; if (tail == queueSize) tail = 0;
        bytesInQueue_ -= tbSize;
        computeSizeOfLastTb();
        return true;
    }
}
int32_t TbCalQueue::bytesInQueue() {
    return bytesInQueue_;
}
int64_t TbCalQueue::getDelay(int64_t time_us) {
    if (items[tail]->used == false) {
        return 0;
    } else {
        return time_us-items[tail]->ts + 500;
    }
}
void TbCalQueue::clear() {
    for (int n=0; n < queueSize; n++) {
        items[n]->used = false;  
    }
    head = -1;
    tail = 0;
    nItems = 0;
    bytesInQueue_ = 0;
    sizeOfLastTb_ = -1;
}
void TbCalQueue::computeSizeOfLastTb() {
    if (!items[tail]->used) {
        sizeOfLastTb_ = - 1;
    } else {
        sizeOfLastTb_ = items[tail]->tbSize;
    }
}
void TbCalQueue::calSchedRate(int64_t time_us) {
    if(bytesInQueue_ < 1000 || time_us < 108000){return;}
    adjustSchedQueue(time_us);
    if(bytesInQueue_ != 0) {
        //cout<<"Time: "<<time_us<<" bytresInQueue: "<<bytesInQueue_<<" Delay: "<<getDelay(time_us)<<endl;;
        m_schedRate = bytesInQueue() *8 *1e6 / getDelay(time_us);
    }  
    
}
bool TbCalQueue:: adjustSchedQueue(int64_t time_us) {
    if(bytesInQueue() == 0) {
        return false;
    } else {
        int32_t tbSize;
        while(items[tail]->ts < time_us-rateWindow) {pop(tbSize);}
    }
}

//hSendRateQueueItem methods
HSendRateQueueItem::HSendRateQueueItem()
{
    sendRate = 0;//bps
}
//HSendRateQueue methods
HSendRateQueue::HSendRateQueue() {
    for (int n=0; n < queueSize; n++) {
        items[n] = new HSendRateQueueItem();
    }
    head = -1;
    tail = 0;
    nItems = 0;
}
void HSendRateQueue::push(int64_t ts, float sendRate) {
    head++; if (head == queueSize) head = 0;
    items[head]->ts = ts;
    items[head]->sendRate = sendRate;
    nItems += 1;
}
bool HSendRateQueue::pop() {
    if (nItems == 0) {
        return false;
    } else {
        tail++; if (tail == queueSize) tail = 0;
        nItems -= 1;
        return true;
    }
}
bool HSendRateQueue::updateRateQueue(int64_t time_us) {
    if(nItems == 1 && time_us < items[tail]->ts){return false;}
    else{
        while(time_us >= items[tail+1]->ts) {
            pop();
            if(nItems < 1) {return  false;}
        }
    }
    return true;
}
float HSendRateQueue::getSendRate() {
    return items[head]->sendRate;
}

float HSendRateQueue::getHistRate(int64_t time_us) {
    if(updateRateQueue(time_us)) {
        return items[tail]->sendRate;
    }
    else {return 0;}
}

//FbInfoQueueItem methods
FbInfoQueueItem::FbInfoQueueItem() {
    int64_t timestamp = 0;
    uint64_t seqNr = 0;
    int16_t incPac = 0;
}
//FbInfoQueue methods
FbInfoQueue::FbInfoQueue() {
    for (int n=0; n < queueSize; n++) {
        items[n] = new FbInfoQueueItem();
    }
    head = -1;
    tail = 0;
    nItems = 0;
}
void FbInfoQueue::push(int64_t tmsp,uint64_t seqNr,int16_t incPac) {
    head++; if (head == queueSize) head = 0;
    items[head]->timestamp = tmsp;
    items[head]->seqNr = seqNr;
    items[head]->incPac = incPac;
    nItems += 1;
}
bool FbInfoQueue::pop(){
    if (nItems == 0) {
        return false;
    } else {
        tail++; if (tail == queueSize) tail = 0;
        nItems -= 1;
        return true;
    }
}
void FbInfoQueue::updateInfoQueue(){
    if((nItems > 0)){
        while((items[head]->timestamp > (items[tail]->timestamp +  fbInfoEvalWindow))){
            pop();
    }}
}
int64_t FbInfoQueue::getTailTs() {
    return items[tail]->timestamp;
}

}
