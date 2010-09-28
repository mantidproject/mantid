#ifndef H_TEST_DIMENSION
#define H_TEST_DIMENSION

#include <cxxtest/TestSuite.h>
#include "DimensionRes.h"

using namespace Mantid;
using namespace MDDataObjects;
// test class for dimension
class tDimension: public Dimension
{
public:
    tDimension(DimensionsID ID):Dimension(ID){};
    virtual void  setRange(double rMin=-1,double rMax=1,unsigned int nBins=1){
        Dimension::setRange(rMin,rMax,nBins);
    }
    void  setName(const char *name){
          Dimension::setName(name);
    }
    void setIntegrated(){
        Dimension::setIntegrated();
    }
    void setExpanded(unsigned int nBins){
        Dimension::setExpanded(nBins);
    }

};
// test class for dimensionRes
class tDimensionRes: public DimensionRes
{
public:
    tDimensionRes(DimensionsID ID):DimensionRes(ID){};
    virtual void  setRange(double rMin=-1,double rMax=1,unsigned int nBins=1){
        DimensionRes::setRange(rMin,rMax,nBins);
    }
    void  setName(const char *name){
          DimensionRes::setName(name);
    }
    void setIntegrated(){
        DimensionRes::setIntegrated();
    }
    void setExpanded(unsigned int nBins){
        DimensionRes::setExpanded(nBins);
    }

};

class testMDDimension :    public CxxTest::TestSuite
{
public:
    void testDimensionPars(void){
    // this is wrong casting and should not work
      TS_ASSERT_THROWS_ANYTHING(tDimension  Dim1(DimensionsID(-1)));

      try{
          tDimension  Dim1(en);
              // wrong limits
          TS_ASSERT_THROWS_ANYTHING(Dim1.setRange(20,-200,200))
          // wrong bins (too many)
          TS_ASSERT_THROWS_ANYTHING(Dim1.setRange(20,-200,200000000))

          // should be ok -- sets axis and ranges
          TS_ASSERT_THROWS_NOTHING(Dim1.setRange(-200,200,200));
          // should get axis points withour any problem
          std::vector<double> points;
 //Why this fails????         TS_ASSERT_THROWS_NOTHING(Dim1.getAxisPoints(points));

          TS_ASSERT_DELTA(Dim1.getRange(),400,FLT_EPSILON);

          TS_ASSERT_DELTA(Dim1.getMinimum(),-200,FLT_EPSILON);
          TS_ASSERT_DELTA(Dim1.getMaximum(), 200,FLT_EPSILON);

          // check axis name
          const char *NAME="En";
          TS_ASSERT_SAME_DATA(Dim1.getName().c_str(),NAME,3);

          // set axis name
 //Why this fails????      TS_ASSERT_THROWS_NOTHING(Dim1.setName("MY new axis name"));
 //        TS_ASSERT_SAME_DATA(Dim1.getName().c_str(),"MY new axis name",strlen(Dim1.getName().c_str()));

          // is integrated?, false by default nBins > 1 so it is not integrated
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),false);
          // it is now integrated;
          TS_ASSERT_THROWS_NOTHING(Dim1.setIntegrated());
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),true);

          TS_ASSERT_THROWS_ANYTHING(Dim1.setExpanded(10000000000));
          TS_ASSERT_THROWS_NOTHING(Dim1.setExpanded(100));
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),false);

      // axiss
          std::vector<double> ax;
          TS_ASSERT_THROWS_NOTHING(ax=Dim1.getAxis());
          TS_ASSERT_THROWS_NOTHING(ax=Dim1.getCoord());

// are these correct vectors?
          TS_ASSERT_EQUALS(ax.size(),1); 
          TS_ASSERT_DELTA(ax[0],1,FLT_EPSILON);


      }catch(std::exception &err){
          std::cout<<" error of the Dim constructor "<<err.what()<<std::endl;
          TS_ASSERT_THROWS_NOTHING(throw(err));
      }


    }
    void testDimensionRes(void){

          tDimensionRes  Dim0(eh);

          std::vector<double> e0;
          TS_ASSERT_THROWS_NOTHING(e0=Dim0.getCoord());


          // are these correct vectors?
          TS_ASSERT_EQUALS(e0.size(),3);


          TS_ASSERT_DELTA(e0[0],1,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[1],0,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[2],0,FLT_EPSILON);



    }
};
#endif
