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
	void setDirection(const V3D &newDir){
		MDDimensionRes::setDirection(newDir);
	}

};

class MDDimensionTest :    public CxxTest::TestSuite
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
	void testDimConstrFromMDBD(){
		std::auto_ptr<MDBasisDimension> pBasDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("xx",true,1,"",V3D(1,1,0)));
		TSM_ASSERT_THROWS_NOTHING("Correct constructor should not throw",std::auto_ptr<MDDimension> pDim=std::auto_ptr<MDDimension>(new MDDimensionRes(*pBasDim)));
	}
	void testDimConstrFromMDBDThrows1(){
		std::auto_ptr<MDBasisDimension> pBasDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("xx",false,1,"",V3D(0,0,0)));
		TSM_ASSERT_THROWS("Reciprocal dimension can not be intiated by non-reciprocal basis dimension",std::auto_ptr<MDDimension> pDim=std::auto_ptr<MDDimension>(new MDDimensionRes(*pBasDim)),std::invalid_argument);
	}
	void testDimConstrFromMDBDThrows2(){
		std::auto_ptr<MDBasisDimension> pBasDim = std::auto_ptr<MDBasisDimension>(new MDBasisDimension("xx",true,3,"",V3D(1,1,0)));
		TSM_ASSERT_THROWS("Reciprocal dimension can not be intiated by a column with number not from (0,1,2)",std::auto_ptr<MDDimension> pDim=std::auto_ptr<MDDimension>(new MDDimensionRes(*pBasDim)),std::invalid_argument);
	}

	void testDirections(void){
		V3D dirOrt = pOrtDim->getDirection();
		V3D dirRec = pResDim->getDirection();
		
		TSM_ASSERT_DELTA("The norm for the orthogonal dimension direction should be close to 0",0.0,dirOrt.norm2(),FLT_EPSILON);
		TSM_ASSERT_DELTA("The norm for the reciprocal dimension direction should be close to 1",1.0,dirRec.norm2(),FLT_EPSILON);
		V3D desDir(1,0,0);
		TSM_ASSERT_EQUALS("First reciprocal dimension direction should be {1,0,0}",desDir,dirRec);
	}
	void testZeroDirectionThrows(){
		V3D zeroDir;
		TSM_ASSERT_THROWS("A direction in a reciprocal dimension can not be 0",pResDim->setDirection(zeroDir),std::invalid_argument);
	}
	void testSetDirection(void){
		V3D desDir(1,-2,0);
		TSM_ASSERT_THROWS_NOTHING("Setting an direction should not throw",pResDim->setDirection(desDir));

		TSM_ASSERT_DELTA("The norm for the reciprocal dimension direction should be close to 1",1.0,pResDim->getDirection().norm2(),FLT_EPSILON);

		TSM_ASSERT_EQUALS("The actual reciprocal dimension should be as set ",desDir,pResDim->getDirectionCryst());

		desDir.normalize();
	    TSM_ASSERT_EQUALS("The actual reciprocal dimension should be as set but normalized to 1 ",desDir,pResDim->getDirection());

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
      if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");

      double x;
      TS_ASSERT_THROWS_NOTHING(x=pOrtDim->getX(0));
      TS_ASSERT_DELTA(x,pOrtDim->getMinimum(),FLT_EPSILON);

      size_t nBins;
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
         TS_ASSERT_SAME_DATA(pOrtDim->getName().c_str(),"MY new axis name",static_cast<unsigned int>(strlen(pOrtDim->getName().c_str())));

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

    }
	  void testRecDimDirection(){
        if(!pOrtDim)TS_FAIL("pOrtDim class has not been constructed properly");
      // axiss
          V3D dir;
          TS_ASSERT_THROWS_NOTHING(dir=pResDim->getDirectionCryst());
          TS_ASSERT_DELTA(dir[0],1,FLT_EPSILON);
	  }

    void testDimensionRes(void){


        tDimensionRes dimY("yy",q2);
        V3D e0;
		TS_ASSERT_THROWS_NOTHING(e0=dimY.getDirection());

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

    MDDimensionTest():pResDim(NULL),pOrtDim(NULL){}
    ~MDDimensionTest(){
        if(pResDim)delete pResDim;
        if(pOrtDim)delete pOrtDim;
        pResDim=NULL;
        pOrtDim=NULL;
    }

    /// Tests for orthogonal dimensions
    void testToXMLStringIntegrated()
    {
      tDimension dimension("1");
      dimension.setRange(1, 3, 1); // last argument gives 1 bin, so not integrated.
      dimension.setName("Qx");
      dimension.setIntegrated();

      //Expected output of the xml serialization code.
      std::string expectedXML =std::string( 
      "<Dimension ID=\"1\">") +
      "<Name>Qx</Name>" +
      "<UpperBounds>3</UpperBounds>" +
      "<LowerBounds>1</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
      "<Integrated>" +
        "<UpperLimit>3</UpperLimit>" +
        "<LowerLimit>1</LowerLimit>" +
      "</Integrated>" +
      "</Dimension>";

      TSM_ASSERT_EQUALS("The xml generated does not meet the schema.", expectedXML, dimension.toXMLString());
    }

    /// Test for orthogonal dimensions
    void testToXMLStringNotIntegrated()
    {
      tDimension dimension("1");
      dimension.setRange(1, 3, 10); // last argument gives 1 bin.
      dimension.setName("Qx");

      //Expected output of the xml serialization code.
      std::string expectedXML =std::string( 
      "<Dimension ID=\"1\">") +
      "<Name>Qx</Name>" +
      "<UpperBounds>3</UpperBounds>" +
      "<LowerBounds>1</LowerBounds>" +
      "<NumberOfBins>10</NumberOfBins>" +
      "</Dimension>";

      TSM_ASSERT_EQUALS("The xml generated does not meet the schema.", expectedXML, dimension.toXMLString());
    }

    /// Test for reciprocal dimensions Q1
    void testToXMLStringReciprocalQ1()
    {
      tDimensionRes dimension("1", q1);
      dimension.setRange(1, 3, 1); // last argument gives 1 bin, so not integrated.
      dimension.setName("Qx");
      dimension.setIntegrated();

      //Expected output of the xml serialization code.
      std::string expectedXML =std::string( 
      "<Dimension ID=\"1\">") +
      "<Name>Qx</Name>" +
      "<UpperBounds>3</UpperBounds>" +
      "<LowerBounds>1</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
      "<Integrated>" +
        "<UpperLimit>3</UpperLimit>" +
        "<LowerLimit>1</LowerLimit>" +
      "</Integrated>" +
      "<ReciprocalDimensionMapping>q1</ReciprocalDimensionMapping>" + // NB: q1 in expectation
      "</Dimension>";

      TSM_ASSERT_EQUALS("The xml generated does not meet the schema.", expectedXML, dimension.toXMLString());
    }

    /// Test for reciprocal dimensions Q2
    void testToXMLStringReciprocalQ2()
    {
      tDimensionRes dimension("1", q2);
      dimension.setRange(1, 3, 1); // last argument gives 1 bin, so not integrated.
      dimension.setName("Qy");
      dimension.setIntegrated();

      //Expected output of the xml serialization code.
      std::string expectedXML =std::string( 
      "<Dimension ID=\"1\">") +
      "<Name>Qy</Name>" +
      "<UpperBounds>3</UpperBounds>" +
      "<LowerBounds>1</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
      "<Integrated>" +
        "<UpperLimit>3</UpperLimit>" +
        "<LowerLimit>1</LowerLimit>" +
      "</Integrated>" +
      "<ReciprocalDimensionMapping>q2</ReciprocalDimensionMapping>" + // NB: q2 in expectation
      "</Dimension>";

      TSM_ASSERT_EQUALS("The xml generated does not meet the schema.", expectedXML, dimension.toXMLString());
    }

    /// Test for reciprocal dimensions Q3
    void testToXMLStringReciprocalQ3()
    {
      tDimensionRes dimension("1", q3);
      dimension.setRange(1, 3, 1); // last argument gives 1 bin, so not integrated.
      dimension.setName("Qz");
      dimension.setIntegrated();

      //Expected output of the xml serialization code.
      std::string expectedXML =std::string( 
      "<Dimension ID=\"1\">") +
      "<Name>Qz</Name>" +
      "<UpperBounds>3</UpperBounds>" +
      "<LowerBounds>1</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
      "<Integrated>" +
        "<UpperLimit>3</UpperLimit>" +
        "<LowerLimit>1</LowerLimit>" +
      "</Integrated>" +
      "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" + // NB: q3 in expectation
      "</Dimension>";

      TSM_ASSERT_EQUALS("The xml generated does not meet the schema.", expectedXML, dimension.toXMLString());
    }

};
#endif
