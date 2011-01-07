#ifndef MDDATA_POINT_TEST_H
#define MDDATA_POINT_TEST_H
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDDataPoints.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace Mantid;
using namespace MDDataObjects;

struct data4D
{
  float q1,q2,q3,En;
  double S,Err;
  int   iRun,iPix,iEn;
};
struct data5D
{
  float q1,q2,q3,En,T;
  double S,Err;
  int   iRun,iPix,iEn,iT;
};
struct eventData4D
{
  float q1,q2,q3,En;
  int   iRun,iPix,iEn;
};




class MDDataPointTest :    public CxxTest::TestSuite
{
  std::vector<std::string> fieldNames4D;
  std::vector<std::string> fieldNames5D;
  MDPointStructure descriptor4D;
  MDPointStructure descriptor5D;
  MDPointDescription *pix4D,*pix5D;
  // helper function generating the data for the tests;
  void build4DTestdata(data4D testData[],unsigned int nPixels)
  {
   for(unsigned int i=0;i<nPixels;i++)
    {
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
  }

public:
  void testMDPointDescriptionConstructorsDefault()
  {
    // default constructor builing 4x3 pixel description for histohram data and default tags
    TS_ASSERT_THROWS_NOTHING(MDPointDescription defSig);
  }

  void testMDPointDescrFromFields()
  {
    // and here we have default 4x3 histohram data itself 
    MDPointStructure defaultPoint;
    // and build pixel description from the default point using default tags; 
    TS_ASSERT_THROWS_NOTHING(MDPointDescription def(defaultPoint));
  }

  void testMDPointDescriptionFromFieldsAndTags()
  {
    MDPointStructure defaultPoint;
    // now let's have "non-default" tags
    std::vector<std::string> tags = fieldNames4D;
    // define the pixel from these tags and 
    TS_ASSERT_THROWS_NOTHING(MDPointDescription def(defaultPoint,tags));
  }

  void testEventDataDescription()
  {
    MDPointStructure defaultPoint;
    // let's set default point for describing expanded event data
    defaultPoint.NumDataFields = 0;
    // should be ok
    TS_ASSERT_THROWS_NOTHING(MDPointDescription def(defaultPoint));
  }

  void testTagsFieldConsistency()
  {
    // let's set default point for describing expanded event data
    MDPointStructure defaultPoint;
    defaultPoint.NumDataFields = 0;
    // and have "non-default" tags for TOF data
    std::vector<std::string> tags = fieldNames4D;
    // names and fields are not-consistent
    TS_ASSERT_THROWS_ANYTHING(MDPointDescription def(defaultPoint,tags));
    tags.erase(tags.begin()+4,tags.begin()+6);
    // now it should work;
    TS_ASSERT_THROWS_NOTHING(MDPointDescription def(defaultPoint,tags));
  }

  void testMDPoint4fx1dx2u8Constructors()
  {
    MDDataPoint<float,uint8_t>  *pPoints;
    char *dataBuf(NULL);
    // this constructor acutally owerwirtes default value for 8-bit fields as a TOF-kind of pixel is constructed which redefines 2 first fields as 32-bit field; MESS!
    TS_ASSERT_THROWS_NOTHING(pPoints = (new MDDataPoint<float,uint8_t>(dataBuf,4,1,2)));

    TS_ASSERT_EQUALS(pPoints->getColumnNames().size(),pPoints->getNumPointFields());
    TS_ASSERT_EQUALS(pPoints->getNumDimensions(),4);
    TS_ASSERT_EQUALS(pPoints->getNumSignals(),1);
    TS_ASSERT_EQUALS(pPoints->getNumDimIndex(),2);
    TS_ASSERT_EQUALS(pPoints->sizeofMDDataPoint(),4*sizeof(float)+1*sizeof(double)+sizeof(uint32_t));

    delete pPoints;
    pPoints=NULL;
  }


  void testMDPointDefaultConstructor()
  {
    MDDataPoint<float,uint16_t> *pPoint;
    char *dataBuf(NULL);
    MDPointStructure defaultPoint;
    MDPointDescription sig(defaultPoint,fieldNames4D);

    TS_ASSERT_THROWS_NOTHING(pPoint = (new MDDataPoint<float,uint16_t>(dataBuf,sig)));

    TS_ASSERT_EQUALS(pPoint->getColumnNames().size(),pPoint->getNumPointFields());
    TS_ASSERT_EQUALS(pPoint->getNumDimensions(),4);
    TS_ASSERT_EQUALS(pPoint->getNumSignals(),2);
    TS_ASSERT_EQUALS(pPoint->getNumDimIndex(),3);
    TS_ASSERT_EQUALS(pPoint->sizeofMDDataPoint(),4*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+sizeof(uint16_t));

    delete pPoint;
  }

  void test4DAccess()
  {
    const int nPix = 10;
    int i;
    data4D testData[nPix];
    char   testBuffer[(nPix+1)*(4*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+sizeof(uint16_t))];

	build4DTestdata(testData,nPix);
   
    TS_ASSERT_THROWS_NOTHING(pix4D = new MDPointDescription(descriptor4D,fieldNames4D));

    MDDataPoint<float,uint16_t>  PackUnpacker(testBuffer,*pix4D);

    float  Dim[4];
    double SE[2];
    int    Ind[3];
    for(i=0;i<nPix;i++)
    {
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
    // testing maximal index and run number
    Ind[0]=(uint32_t)std::pow(2.,10)-1; //1023=(2^10-1)
    Ind[1]=(uint32_t)std::pow(2.,32-10)-1; // 4194303 = (2^22-1)
    PackUnpacker.setData(nPix,Dim,SE,Ind);
    TS_ASSERT_EQUALS(PackUnpacker.getRunID(nPix), Ind[0]);
    TS_ASSERT_EQUALS(PackUnpacker.getPixID(nPix), Ind[1]);

    for(i=0;i<nPix;i++)
    {
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

  void testHoracePointAccess(){
   const int nPix = 10;
    int i;
    data4D testData[nPix];
    char   testBuffer[nPix*(4*sizeof(double)+2*sizeof(double)+3*sizeof(uint64_t))];

    build4DTestdata(testData,nPix);
	MDPointStructure horStruct;

	horStruct.DimIDlength=8;
	horStruct.DimLength  =8;
	horStruct.NumPixCompressionBits=0;

	MDPointDescription HorDescription(horStruct,fieldNames4D);

	MDDataPointEqual<float,uint32_t,float> hp(testBuffer,HorDescription);

    float     Dim[4];
    float     SE[2];
    uint32_t   Ind[3];
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
      hp.setData(i,Dim,SE,Ind);

    }

   for(i=0;i<nPix;i++)
    {
      TS_ASSERT_EQUALS(hp.getDataField(0,i),testData[i].q1);
      TS_ASSERT_EQUALS(hp.getDataField(1,i),testData[i].q2);
      TS_ASSERT_EQUALS(hp.getDataField(2,i),testData[i].q3);
      TS_ASSERT_EQUALS(hp.getDataField(3,i),testData[i].En);
      TS_ASSERT_EQUALS(hp.getSignal(i),testData[i].S);
      TS_ASSERT_EQUALS(hp.getError(i), testData[i].Err);

      TS_ASSERT_EQUALS(hp.getIndex(0,i), testData[i].iRun);
      TS_ASSERT_EQUALS(hp.getIndex(1,i), testData[i].iPix);
      TS_ASSERT_EQUALS(hp.getIndex(2,i), testData[i].iEn);

    }


  }
  void test4DAccess32bitIndex()
  {
    const int nPix = 10;
    int i;
    data4D testData[nPix];
    char   testBuffer[nPix*(4*sizeof(float)+2*sizeof(double)+2*sizeof(uint32_t))];

    build4DTestdata(testData,nPix);

    MDDataPoint<float,uint32_t>  PackUnpacker(testBuffer,*pix4D);

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

  void test5DAccess(void)
  {
    const int nPix = 10;
    int i;
    data5D testData[nPix];
    char   testBuffer[nPix*(5*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+2*sizeof(uint16_t))];
    for(i=0;i<10;i++)
    {
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

    TS_ASSERT_THROWS_NOTHING(pix5D = new MDPointDescription(descriptor5D,fieldNames5D));

    MDDataPoint<float,uint16_t>  PackUnpacker(testBuffer,*pix5D);

    float  Dim[5];
    double SE[2];
    int    Ind[4];
    for(i=0;i<nPix;i++)
    {
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

    for(i=0;i<nPix;i++)
    {
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

  void testPixelCopying(){
  const int nPix = 10;
    int i;
    data4D testData[nPix];
    char   sourceBuffer[(nPix)*(4*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+sizeof(uint16_t))];

    build4DTestdata(testData,nPix);

    TS_ASSERT_THROWS_NOTHING(pix4D = new MDPointDescription(descriptor4D,fieldNames4D));

    MDDataPoint<float,uint16_t>  PackUnpacker(sourceBuffer,*pix4D);

    float  Dim[4];
    double SE[2];
    int    Ind[3];
    for(i=0;i<nPix;i++)
    {
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
    char   targetBuffer[(2*nPix)*(4*sizeof(float)+2*sizeof(double)+sizeof(uint32_t)+sizeof(uint16_t))];

	for(i=0;i<nPix;i++){
		PackUnpacker.copyPixel(i,targetBuffer,2*i);
	}
	// initate path-through class to interpret the transformed data
    MDDataPoint<float,uint16_t>  newUnpacker(targetBuffer,*pix4D);
 // 
    for(i=0;i<nPix;i++)
    {
      TS_ASSERT_EQUALS(newUnpacker.getDataField(0,2*i),testData[i].q1);
      TS_ASSERT_EQUALS(newUnpacker.getDataField(1,2*i),testData[i].q2);
      TS_ASSERT_EQUALS(newUnpacker.getDataField(2,2*i),testData[i].q3);
      TS_ASSERT_EQUALS(newUnpacker.getDataField(3,2*i),testData[i].En);
      TS_ASSERT_EQUALS(newUnpacker.getSignal(2*i),testData[i].S);
      TS_ASSERT_EQUALS(newUnpacker.getError(2*i), testData[i].Err);
      TS_ASSERT_EQUALS(newUnpacker.getRunID(2*i), testData[i].iRun);
      TS_ASSERT_EQUALS(newUnpacker.getPixID(2*i), testData[i].iPix);

      TS_ASSERT_EQUALS(newUnpacker.getIndex(2,2*i), testData[i].iEn);

    }

  }
  void testEventData4D(void)
  {
    const int nPix = 10;
    int i;
    eventData4D testData[nPix];
    char   testBuffer[nPix*(4*sizeof(float)+sizeof(uint32_t)+sizeof(uint16_t))];
    for(i=0;i<10;i++)
    {
      testData[i].q1  =(float)(i+1);
      testData[i].q2  =(float)(i+1)*2;
      testData[i].q3  =(float)(i+1)*3;
      testData[i].En  =(float)(i+1)*4;
      testData[i].iRun=(i+1)*7;
      testData[i].iPix=(i+1)*8;
      testData[i].iEn =(i+1)*9; 
    }
    delete pix4D;
    descriptor4D.NumDataFields=0;
    pix4D  = NULL;
    fieldNames4D.erase(fieldNames4D.begin()+4,fieldNames4D.begin()+6);

    TS_ASSERT_THROWS_NOTHING(pix4D = new MDPointDescription(descriptor4D,fieldNames4D));
    if(!pix4D)return;

    MDDataPoint<float,uint16_t>  PackUnpacker(testBuffer,*pix4D);

    float  Dim[4];
    double SE[2];
    int    Ind[3];
    for(i=0;i<nPix;i++)
    {
      Dim[0]=testData[i].q1;
      Dim[1]=testData[i].q2;
      Dim[2]=testData[i].q3;
      Dim[3]=testData[i].En;
      Ind[0] =testData[i].iRun ;
      Ind[1] =testData[i].iPix ;
      Ind[2] =testData[i].iEn ;
      PackUnpacker.setData(i,Dim,SE,Ind);
    }

    for(i=0;i<nPix;i++)
    {
      TS_ASSERT_EQUALS(PackUnpacker.getDataField(0,i),testData[i].q1);
      TS_ASSERT_EQUALS(PackUnpacker.getDataField(1,i),testData[i].q2);
      TS_ASSERT_EQUALS(PackUnpacker.getDataField(2,i),testData[i].q3);
      TS_ASSERT_EQUALS(PackUnpacker.getDataField(3,i),testData[i].En);
      TS_ASSERT_EQUALS(PackUnpacker.getRunID(i), testData[i].iRun);
      TS_ASSERT_EQUALS(PackUnpacker.getPixID(i), testData[i].iPix);
      TS_ASSERT_EQUALS(PackUnpacker.getIndex(2,i), testData[i].iEn);
    }
  }


  MDDataPointTest()
  {
    const char *cfieldNames4D[]={"q1","q2","q3","En","S","Err","iRun","iPix","iEn"};
    const char *cfieldNames5D[]={"q1","q2","q3","En","T","S","Err","iRun","iPix","iEn","iT"};
    fieldNames4D.assign(cfieldNames4D,(cfieldNames4D+9));
    fieldNames5D.assign(cfieldNames5D,(cfieldNames5D+11));

    descriptor5D.NumDimensions=5;
    descriptor5D.NumDimIDs= 4;
  }


  ~MDDataPointTest()
  {
    delete pix4D;
    delete pix5D;
  }
};

#endif
