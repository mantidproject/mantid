#ifndef H_TEST_DIMENSION
#define H_TEST_DIMENSION

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include <cfloat>
#include <cstring>

using namespace Mantid;
using namespace Geometry;
// test class for dimension; The dimensions are internal classes for MD geometry;
class tDimension: public MDDimension
{
public:
    tDimension(const std::string &ID):MDDimension(ID){};
    virtual void  setRange(double rMin=-1,double rMax=1,unsigned int nBins=1){
        MDDimension::setRange(rMin,rMax,nBins);
    }
    void  setName(const char *name){
          MDDimension::setName(name);
    }
    void  setName(const std::string & name){
          MDDimension::setName(name);
    }
    void setIntegrated(){
        MDDimension::setIntegrated();
    }
    void setExpanded(unsigned int nBins){
        MDDimension::setExpanded(nBins);
    }

};
// test class for dimensionRes
class tDimensionRes: public MDDimensionRes
{
public:
   tDimensionRes(const std::string &ID,const rec_dim nDim):MDDimensionRes(ID,nDim){}; 
   virtual void  setRange(double rMin=-1,double rMax=1,unsigned int nBins=1){
        MDDimensionRes::setRange(rMin,rMax,nBins);
    }
    void  setName(const char *name){
          MDDimensionRes::setName(name);
    }
    void setIntegrated(){
        MDDimensionRes::setIntegrated();
    }
    void setExpanded(unsigned int nBins){
        MDDimensionRes::setExpanded(nBins);
    }

};

class testMDDimension :    public CxxTest::TestSuite
{
    tDimensionRes *pResDim;
    tDimension    *pOrtDim;
public:

    void testPublicConstructor()
	{
	 using namespace Mantid::Geometry;
	 std::string id = "1";
	 MDDimension dim(id); //Will cause compilation error if constructor is hidden.
	 TSM_ASSERT_EQUALS("Id getter not wired-up correctly.", "1", dim.getDimensionId());
	}

    void testDimensionConstructor(void){
        // define one reciprocal 
       TS_ASSERT_THROWS_NOTHING(pResDim = new tDimensionRes("x",q1));
       // and one orthogonal dimension
       TS_ASSERT_THROWS_NOTHING(pOrtDim = new tDimension("en"));
    }
    void testSetRanges(){
        if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");

          // wrong limits
          TS_ASSERT_THROWS_ANYTHING(pOrtDim->setRange(20,-200,200))
          // wrong bins (too many)
          TS_ASSERT_THROWS_ANYTHING(pOrtDim->setRange(-20,200, 2 * MAX_REASONABLE_BIN_NUMBER))

          // should be ok -- sets axis and ranges
          TS_ASSERT_THROWS_NOTHING(pOrtDim->setRange(-200,200,200));
          // should get axis points withour any problem
          std::vector<double> points;
          TS_ASSERT_THROWS_NOTHING(pOrtDim->getAxisPoints(points));

          TS_ASSERT_DELTA(pOrtDim->getRange(),400,FLT_EPSILON);

          TS_ASSERT_DELTA(pOrtDim->getMinimum(),-200,FLT_EPSILON);
          TS_ASSERT_DELTA(pOrtDim->getMaximum(), 200,FLT_EPSILON);

          // check axis name
          TS_ASSERT_EQUALS(pOrtDim->getName(),"en");
    }
    void testGetX(){
      double x;
      if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");
      TS_ASSERT_THROWS_NOTHING(x=pOrtDim->getX(0));
      TS_ASSERT_DELTA(x,pOrtDim->getMinimum(),FLT_EPSILON);
      unsigned int nBins;
      TS_ASSERT_THROWS_NOTHING(nBins = pOrtDim->getNBins());

      TS_ASSERT_THROWS_NOTHING(x=pOrtDim->getX(nBins));
      TS_ASSERT_DELTA(x,pOrtDim->getMaximum(),FLT_EPSILON);
      // out of range request
      TS_ASSERT_THROWS_ANYTHING(x=pOrtDim->getX(-1));
      TS_ASSERT_THROWS_ANYTHING(x=pOrtDim->getX(nBins+1));
    }

    void testSetAxisName(){
        if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");
          // set axis name

         std::string name("MY new axis name");
         TS_ASSERT_THROWS_NOTHING(pOrtDim->setName(name));   
         TS_ASSERT_SAME_DATA(pOrtDim->getName().c_str(),"MY new axis name",strlen(pOrtDim->getName().c_str()));

          // is integrated?, false by default nBins > 1 so it is not integrated
          TS_ASSERT_EQUALS(pOrtDim->getIntegrated(),false);
          // it is now integrated;
          TS_ASSERT_THROWS_NOTHING(pOrtDim->setIntegrated());
          TS_ASSERT_EQUALS(pOrtDim->getIntegrated(),true);
          // the n-bins is too high
          TS_ASSERT_THROWS_ANYTHING(pOrtDim->setExpanded(MAX_REASONABLE_BIN_NUMBER+10));
          // this one should be fine. 
          TS_ASSERT_THROWS_NOTHING(pOrtDim->setExpanded(100));
          TS_ASSERT_EQUALS(pOrtDim->getIntegrated(),false);
    }
    void testAxis(){
        if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");
      // axiss
          std::vector<double> ax;
          TS_ASSERT_THROWS_NOTHING(ax=pResDim->getAxis());
          TS_ASSERT_THROWS_NOTHING(ax=pResDim->getCoord());

// are these correct vectors?
          TS_ASSERT_EQUALS(ax.size(),3); 
          TS_ASSERT_DELTA(ax[0],1,FLT_EPSILON);

          TS_ASSERT_THROWS_NOTHING(ax=pOrtDim->getAxis());
          TS_ASSERT_THROWS_NOTHING(ax=pOrtDim->getCoord());

// are these correct vectors?
          TS_ASSERT_EQUALS(ax.size(),1); 
          TS_ASSERT_DELTA(ax[0],1,FLT_EPSILON);

    }

    void testDimensionRes(void){


        tDimensionRes dimY("yy",q2);
        std::vector<double> e0;
        TS_ASSERT_THROWS_NOTHING(e0=dimY.getCoord());


          // are these correct vectors?
          TS_ASSERT_EQUALS(e0.size(),3);


          TS_ASSERT_DELTA(e0[0],0,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[1],1,FLT_EPSILON);
          TS_ASSERT_DELTA(e0[2],0,FLT_EPSILON);

    }

    void testEquivalent()
    {
      MDDimension a("a");
      MDDimension b("a");
      TSM_ASSERT("Equivelant comparison failed", a == b);
    }

    void testNotEquivalent()
    {
      MDDimension a("a");
      MDDimension b("b");
      TSM_ASSERT("Not Equivelant comparison failed", a != b);
    }

    testMDDimension():pResDim(NULL),pOrtDim(NULL){}
    ~testMDDimension(){
        if(pResDim)delete pResDim;
        if(pOrtDim)delete pOrtDim;
        pResDim=NULL;
        pOrtDim=NULL;
    }


};
#endif
