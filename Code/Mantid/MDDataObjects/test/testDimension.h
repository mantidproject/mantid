#ifndef H_TEST_DIMENSION
#define H_TEST_DIMENSION

#include <cxxtest/TestSuite.h>
#include "DimensionRes.h"

class testDimension :    public CxxTest::TestSuite
{
public:
    void testDimensionPars(void){
    // this is wrong casting and should not work
      TS_ASSERT_THROWS_ANYTHING(Dimension  Dim1(DimensionsID(-1)));

      try{
          Dimension  Dim1(en);
          // wrong limits
          TS_ASSERT_THROWS_ANYTHING(Dim1.setRange(20,-200,200))
          // wrong bins (too many)
          TS_ASSERT_THROWS_ANYTHING(Dim1.setRange(20,-200,200000000))

          // should be ok -- sets axis and ranges
          TS_ASSERT_THROWS_NOTHING(Dim1.setRange(-200,200,200));
          // should get axis points withour any problem
          TS_ASSERT_THROWS_NOTHING(std::vector<double> points=Dim1.getAxisPoints());

          TS_ASSERT_DELTA(Dim1.getRange(),400,FLT_EPSILON);

          TS_ASSERT_DELTA(Dim1.getMinimum(),-200,FLT_EPSILON);
          TS_ASSERT_DELTA(Dim1.getMaximum(), 200,FLT_EPSILON);

          // check axis name
          const char *NAME="En";
          TS_ASSERT_SAME_DATA(Dim1.getName().c_str(),NAME,3);

          // set axis name
          TS_ASSERT_THROWS_NOTHING(Dim1.setName("MY new axis name"));
          TS_ASSERT_SAME_DATA(Dim1.getName().c_str(),"MY new axis name",strlen(Dim1.getName().c_str()));

          // is integrated?, false by default nBins > 1 so it is not integrated
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),false);
          // it is now integrated;
          TS_ASSERT_THROWS_NOTHING(Dim1.setIntegrated());
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),true);

          TS_ASSERT_THROWS_ANYTHING(Dim1.setExpanded(10000000000));
          TS_ASSERT_THROWS_NOTHING(Dim1.setExpanded(100));
          TS_ASSERT_EQUALS(Dim1.getIntegrated(),false);


          DimensionRes  Dim0(eh);
          std::vector<double> e0,e4;
          TS_ASSERT_THROWS_NOTHING(e0=Dim0.getCoord());
          TS_ASSERT_THROWS_NOTHING(e4=Dim1.getCoord());

          // are these correct vectors?
          TS_ASSERT_EQUALS(e0.size(),3);
          TS_ASSERT_EQUALS(e4.size(),1); 

          TS_ASSERT_DELTA(e0[0],1,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[1],0,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[2],0,FLT_EPSILON);
          TS_ASSERT_DELTA(e4[0],1,FLT_EPSILON);

      }catch(errorMantid &err){
          std::cout<<" error of the Dim constructor "<<err.what()<<std::endl;
          TS_ASSERT_THROWS_NOTHING(throw(err));
      }


    }
};
#endif
