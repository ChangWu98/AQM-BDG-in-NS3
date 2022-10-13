/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include <string>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <memory.h>
#include <stdio.h>
#include "ns3/core-module.h"
#include "ns3/throughputRW-helper.h"
using namespace std;
using namespace ns3;
#define MAXUSERNUM 40
#define MAXPACKET 18000

long throughPut[8][MAXUSERNUM];
float packetDdelay[MAXPACKET][2];
char buf[FILENAME_MAX];

bool ReadThroughput(string throughputFullPath,long*throughPut,int ueNum){
  fstream fin(throughputFullPath);
    if(!fin){
      cout<<"N0 Such File!"<<endl;
      return false;
    }
    else{
      for(int i=0;i<8;i++)
      {
        for(int j=0;j<ueNum;j++)
          fin>>*(throughPut+i*MAXUSERNUM+j);
      }
      fin.close();
      return true;
    }
}

void WriteRealThroughputToFile(long(*realThroughput)[MAXUSERNUM],int ueNum){
    char buf[FILENAME_MAX];
    string thoughpuPath=string (getcwd(buf, FILENAME_MAX)) + "/traces/" + "UeNum"+to_string(ueNum)+"/realThoughput.txt";
    ofstream fout(thoughpuPath);
    for(int i=0;i<8;i++){
        for(uint32_t j=0;j<ueNum;j++){
            fout<<realThroughput[i][j]<<" ";
        }
        fout<<"\n";
    }
    fout.close();
}

int 
main (int argc, char *argv[])
{
  string delaypath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" ;//+ filename+"_delay.txt";
  string throughputPath=string (getcwd(buf, FILENAME_MAX)) + "/traces/" + "UeNum";//+to_string(m_userNum)+"/thoughput.txt";
  float timeSec;int delayDua;bool hasOutTime;int checkOutTimeSec;
  for(int ueNum=3;ueNum<=3;ueNum+=1)
  {
    string throughputFullPath=throughputPath+to_string(ueNum)+"/thoughput.txt";
    if(ReadThroughput(throughputFullPath,(long*)throughPut,ueNum)){
      for(int screamUser=1;screamUser<=ueNum;screamUser++){
        checkOutTimeSec=8;hasOutTime=false;
        string delayFullPath=delaypath+"UeNum"+to_string(ueNum)+"/screamUser"
              +to_string(screamUser)+"_delay.txt";
        fstream delayfin(delayFullPath);
        if(!delayfin){
          cout<<"No Such File!";
          return 0;
        }
        else{
          while(!delayfin.eof()){//读取screamUser用户时延文件的每一行
            delayfin>>timeSec;delayfin>>delayDua;
            //cout<<timeSec<<"  "<<delaydua<<endl;
            if(timeSec>float(checkOutTimeSec+1)){
              checkOutTimeSec+=1;
              hasOutTime=false;
            }
            if(delayDua>10&&timeSec>float(checkOutTimeSec)&&timeSec<float(checkOutTimeSec+1)&&hasOutTime==false){
              throughPut[checkOutTimeSec-8][screamUser-1]=0;
              checkOutTimeSec+=1;
              hasOutTime=true;
            }
          }
        }
        delayfin.close();//关闭每个用户的时延文件
      }
      WriteRealThroughputToFile(throughPut,ueNum);
    
      
      // for(int i=0;i<8;i++){
      //   for(int j=0;j<ueNum;j++){
      //       cout<<throughPut[i][j]<<" ";
      //   }
      //   cout<<"\n";
      // }

    }
  }




  // bool verbose = true;

  // CommandLine cmd (__FILE__);
  // cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  // cmd.Parse (argc,argv);

  // /* ... */

  // Simulator::Run ();
  // Simulator::Destroy ();
  return 0;
}


