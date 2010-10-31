#ifndef H_TEST_WORKSPACE_GEOMETRY
#define H_TEST_WORKSPACE_GEOMETRY

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"

using namespace Mantid;
using namespace Geometry;

class publicWorkspaceGeometry: public MDGeometryBasis
{
public:
    publicWorkspaceGeometry(unsigned int nDims=4,unsigned int nReciprocalDims=3):MDGeometryBasis(nDims,nReciprocalDims){};
    int getDimRefNum(const std::string &tag, bool nothrow=false)const{return MDGeometryBasis::getDimIDNum(tag,nothrow);}
   // DimensionID getDimensionID(unsigned int nDim)const{return MDGeometryBasis::getDimensionID(nDim);}
    void reinit_WorkspaceGeometry(const std::vector<std::string> &ID){MDGeometryBasis::reinit_WorkspaceGeometry(ID);}

};

class testWorkspaceGm :   public CxxTest::TestSuite
{
    publicWorkspaceGeometry *pGeometry5x3;
    publicWorkspaceGeometry *pGeom4x2;
    std::vector<std::string> names;
    unsigned int ID2x4,ID3x2;
public:
    void test2x4Basis(){
        TS_ASSERT_THROWS_NOTHING(pGeom4x2= new publicWorkspaceGeometry(4,2));
        TS_ASSERT_THROWS_NOTHING(ID2x4=pGeom4x2->getWorkspaceID());
    }
    void testWorkspaceGeometryConstructor(void){

        // we can not define such dimensions
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(-1));

        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(22));
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(4,4));
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(5,4));
        // the geometry which you can not initiate number of real dimensions lower than number of reciprocal dimensions
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry space(2));

        // now we do define 5-d workspace
        TS_ASSERT_THROWS_NOTHING(pGeometry5x3 = new publicWorkspaceGeometry(5));
        TS_ASSERT_THROWS_NOTHING(ID3x2=pGeometry5x3->getWorkspaceID());

        TS_ASSERT_DIFFERS(ID2x4,ID3x2);
    }
    void testNDim(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

          // we have defined  5 dimension above
        TS_ASSERT_EQUALS((pGeometry5x3->getNumDims()),5);
    }
    void testDefaultNames(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

        TS_ASSERT_THROWS_NOTHING(names=pGeometry5x3->getBasisTags());
        // these tags are:
        const char *defaultTags[]={"hc","kc","lc","en","u1"};
        const char *default4x2Tags[]={"hc","kc","en","u1"};

        for(int i=0;i<5;i++){
            TS_ASSERT_SAME_DATA(names[i].c_str(),defaultTags[i],2);
        }
        TS_ASSERT_THROWS_NOTHING(names=pGeom4x2->getBasisTags());
        for(int i=0;i<4;i++){
            TS_ASSERT_SAME_DATA(names[i].c_str(),default4x2Tags[i],2);
        }
    }
    void testSimpleDimensionID(){
        DimensionID ID1(0,"aa",3);
        DimensionID ID2(1,"bb",3);

        TS_ASSERT_EQUALS(ID1.getDimensionID("bb"),-1);
        TS_ASSERT_EQUALS(ID1.getDimensionID("aa"),0);

       TS_ASSERT_EQUALS(ID2.getDimensionID("blabla"),-1);
       TS_ASSERT_EQUALS(ID2.getDimensionID("x"),-1);
       TS_ASSERT_EQUALS(ID2.getDimensionID("bb"),1);

    }
    void testOrts0(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

      // check if we are getting proper numbers for id-s
        TS_ASSERT_EQUALS((pGeometry5x3->getDimRefNum("hc")),0);
        // this dimension does not exis in 5D workspace
        TS_ASSERT_EQUALS(pGeometry5x3->getDimRefNum("u7",false),-1);
        TS_ASSERT_THROWS_ANYTHING(pGeometry5x3->getDimRefNum("u7",true));

        // attempting to get the coordinate of an non-existing dimension
       TS_ASSERT_THROWS_ANYTHING(pGeometry5x3->getOrt("u7"));

     // this is 3-vector of first dimension
        std::vector<double> e2;
        TS_ASSERT_THROWS_NOTHING(e2=pGeometry5x3->getOrt("kc"));

        // is it realy 3-vector?
        TS_ASSERT_EQUALS(e2.size(),3);
        // is this [0,1,0] ?
        TS_ASSERT_DELTA(e2[1],1,FLT_EPSILON);
        TS_ASSERT_DELTA(e2[0],0,FLT_EPSILON);
        TS_ASSERT_DELTA(e2[2],0,FLT_EPSILON);

        // this is 1-vector of 4-th dimension
        std::vector<double> e4;
        TS_ASSERT_THROWS_NOTHING(e4=pGeometry5x3->getOrt("en"));
        // is this realy a 1-vector?
        TS_ASSERT_EQUALS(e4.size(),1);
        // is this 1?
        TS_ASSERT_DELTA(e4[0],1,FLT_EPSILON);
    }
    void testReinitBasis(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

        // let's try to kill old geometry and build a new one; This option should be availible inside a child class only;
        std::vector<std::string>ID(4,"en");
        // this will initiate the tags for "ereciprocal_energyn" to be a first (0) reciprocal dimension
        ID[0]="reciprocal energy";ID[1]="hc_reciprocal";ID[2]="kc_rec";ID[3]="lc_orthogonal";

        //would not work without any reciprocal dimension, one has to be present;
       TS_ASSERT_THROWS_NOTHING(pGeometry5x3->reinit_WorkspaceGeometry(ID));

       // WorkspaceGeometry dimensions are arranged according to their definition,
        TS_ASSERT_EQUALS(pGeometry5x3->getDimRefNum("reciprocal energy"),0);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimRefNum("hc_reciprocal"),1);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimRefNum("kc_rec"),2);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimRefNum("lc_orthogonal"),3);

        unsigned int id=pGeometry5x3->getWorkspaceID();
        TS_ASSERT_DIFFERS(ID2x4,id);
        TS_ASSERT_DIFFERS(ID3x2,id);

// the techncalities of woriking with 2D+1 and 1D+1 workspaced have not been tested

    }
    testWorkspaceGm():pGeometry5x3(NULL),pGeom4x2(NULL){}
    ~testWorkspaceGm(){
        if( pGeometry5x3){     delete pGeometry5x3;
        }
        if (pGeom4x2){      delete pGeom4x2;
        }
    }
};



#endif
