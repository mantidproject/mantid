#ifndef MDDATA_POINT_TEST_H
#define MDDATA_POINT_TEST_H
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDataPoints.h"

using namespace Mantid;
using namespace MDDataObjects;
struct data4D{
  float q1,q2,q3,En;
  double S,Err;
  int   iRun,iPix,iEn;
};
struct data5D{
  float q1,q2,q3,En,T;
  double S,Err;
  int   iRun,iPix,iEn,iT;
};
struct eventData4D{
  float q1,q2,q3,En;
  int   iRun,iPix,iEn;
};




class testMDDataPoints :    public CxxTest::TestSuite
{
public:
  void test4DAccess(void){
    const int nPix = 10;
    int i;
    data4D testData[nPix];
    char   testBuffer[nPix*(4*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+2)];
    for(i=0;i<10;i++){
      testData[i].q1  =(float)(i+1);
      testData[i].q2  =(float)(i+1)*2;
      testData[i].q3  =(float)(i+1)*3;
      testData[i].En  =(float)(i+1)*4;
      testData[i].S   =(double)(i+1)*5;
      testData[i].Err =(double)(i+1)*6;
      testData[i].iRun=(i+1)*7;
      testData[i].iPix=(i+1)*8;
      testData[i].iEn =(i+1)*9; 
    }
    MDDataPoint<float>  PackUnpacker(testBuffer,4,2,3);

    float  Dim[4];
    double SE[2];
    int    Ind[3];
    for(i=0;i<nPix;i++){
      Dim[0]=testData[i].q1;
      Dim[1]=testData[i].q2;
      Dim[2]=testData[i].q3;
      Dim[3]=testData[i].En;
      SE[0] =testData[i].S ;
      SE[1] =testData[i].Err ;
      Ind[0] =testData[i].iRun ;
      Ind[1] =testData[i].iPix ;
      Ind[2] =testData[i].iEn ;
      PackUnpacker.setData(i,Dim,SE,Ind);

    }

    for(i=0;i<nPix;i++){
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(0,i),testData[i].q1);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(1,i),testData[i].q2);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(2,i),testData[i].q3);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(3,i),testData[i].En);
        TS_ASSERT_EQUALS(PackUnpacker.getSignal(i),testData[i].S);
        TS_ASSERT_EQUALS(PackUnpacker.getError(i), testData[i].Err);
        TS_ASSERT_EQUALS(PackUnpacker.getRunID(i), testData[i].iRun);
        TS_ASSERT_EQUALS(PackUnpacker.getPixID(i), testData[i].iPix);

        TS_ASSERT_EQUALS(PackUnpacker.getIndex(2,i), testData[i].iEn);

    }
  }
         
  void test5DAccess(void){
    const int nPix = 10;
    int i;
    data5D testData[nPix];
    char   testBuffer[nPix*(5*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+2*sizeof(uint16_t))];
    for(i=0;i<10;i++){
      testData[i].q1  =(float)(i);
      testData[i].q2  =(float)(i)*2;
      testData[i].q3  =(float)(i)*3;
      testData[i].En  =(float)(i)*4;
      testData[i].T  =(float)(i)*5;
      testData[i].S   =(double)(i)*6;
      testData[i].Err =(double)(i)*7;
      testData[i].iRun=(i)*8;
      testData[i].iPix=(i)*9;
      testData[i].iEn =(i)*10; 
      testData[i].iT  =(i)*11; 
    }
    MDDataPoint<float>  PackUnpacker(testBuffer,5,2,4);

    float  Dim[5];
    double SE[2];
    int    Ind[4];
    for(i=0;i<nPix;i++){
      Dim[0]=testData[i].q1;
      Dim[1]=testData[i].q2;
      Dim[2]=testData[i].q3;
      Dim[3]=testData[i].En;
      Dim[4]=testData[i].T;
      SE[0] =testData[i].S ;
      SE[1] =testData[i].Err ;
      Ind[0] =testData[i].iRun ;
      Ind[1] =testData[i].iPix ;
      Ind[2] =testData[i].iEn ;
      Ind[3] =testData[i].iT ;
      PackUnpacker.setData(i,Dim,SE,Ind);

    }

    for(i=0;i<nPix;i++){
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(0,i),testData[i].q1);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(1,i),testData[i].q2);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(2,i),testData[i].q3);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(3,i),testData[i].En);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(4,i),testData[i].T);
        TS_ASSERT_EQUALS(PackUnpacker.getSignal(i),testData[i].S);
        TS_ASSERT_EQUALS(PackUnpacker.getError(i), testData[i].Err);
        TS_ASSERT_EQUALS(PackUnpacker.getRunID(i), testData[i].iRun);
        TS_ASSERT_EQUALS(PackUnpacker.getPixID(i), testData[i].iPix);

        TS_ASSERT_EQUALS(PackUnpacker.getIndex(2,i), testData[i].iEn);
        TS_ASSERT_EQUALS(PackUnpacker.getIndex(3,i), testData[i].iT);
    }
         


  }

 void testEventData4D(void){
    const int nPix = 10;
    int i;
    eventData4D testData[nPix];
    char   testBuffer[nPix*(4*sizeof(float)+sizeof(uint32_t)+2)];
    for(i=0;i<10;i++){
      testData[i].q1  =(float)(i+1);
      testData[i].q2  =(float)(i+1)*2;
      testData[i].q3  =(float)(i+1)*3;
      testData[i].En  =(float)(i+1)*4;
      testData[i].iRun=(i+1)*7;
      testData[i].iPix=(i+1)*8;
      testData[i].iEn =(i+1)*9; 
    }
    MDDataPoint<float>  PackUnpacker(testBuffer,4,0,3);

    float  Dim[4];
    double SE[2];
    int    Ind[3];
    for(i=0;i<nPix;i++){
      Dim[0]=testData[i].q1;
      Dim[1]=testData[i].q2;
      Dim[2]=testData[i].q3;
      Dim[3]=testData[i].En;
      Ind[0] =testData[i].iRun ;
      Ind[1] =testData[i].iPix ;
      Ind[2] =testData[i].iEn ;
      PackUnpacker.setData(i,Dim,SE,Ind);

    }

    for(i=0;i<nPix;i++){
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(0,i),testData[i].q1);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(1,i),testData[i].q2);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(2,i),testData[i].q3);
        TS_ASSERT_EQUALS(PackUnpacker.getDataField(3,i),testData[i].En);
        TS_ASSERT_EQUALS(PackUnpacker.getRunID(i), testData[i].iRun);
        TS_ASSERT_EQUALS(PackUnpacker.getPixID(i), testData[i].iPix);

        TS_ASSERT_EQUALS(PackUnpacker.getIndex(2,i), testData[i].iEn);

    }
  }
};

#endif