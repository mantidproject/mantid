#ifndef H_TEST_WORKSPACE_GEOMETRY
#define H_TEST_WORKSPACE_GEOMETRY

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/WorkspaceGeometry.h"

using namespace Mantid;
using namespace MDDataObjects;
class publicWorkspaceGeometry: public WorkspaceGeometry
{
public:
    publicWorkspaceGeometry(unsigned int nDims):WorkspaceGeometry(nDims){};
    DimensionsID getDimensionID(unsigned int nDim)const{return WorkspaceGeometry::getDimensionID(nDim);}
    void reinit_WorkspaceGeometry(const std::vector<DimensionsID> &ID){WorkspaceGeometry::reinit_WorkspaceGeometry(ID);}
    unsigned int getNumDims(void)const{return WorkspaceGeometry::getNumDims();}
};


class testWorkspaceGm :   public CxxTest::TestSuite
{
public:
    void testWorkspaceGeometry(void){

        // we can not define such dimensions
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(-1));

        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry  space1(22));
        // the geometry which is lower than 4 can not be build using this constructor. 
        TS_ASSERT_THROWS_ANYTHING(publicWorkspaceGeometry space(3));
        
        // now we do define 5-d workspace
        TS_ASSERT_THROWS_NOTHING(publicWorkspaceGeometry space(5));


        publicWorkspaceGeometry space(5);
        // the 0 dimension is eh
        TS_ASSERT_EQUALS((space.getDimensionID(0)),eh);

        // we have defined  5 dimension above
        TS_ASSERT_EQUALS((space.getNumDims()),5);
  
        // attempting to get the coordinate of an non-existing dimension
        TS_ASSERT_THROWS_ANYTHING(space.getOrt(u7));

     // this is 3-vector of first dimension
        std::vector<double> e2;
        TS_ASSERT_THROWS_NOTHING(e2=space.getOrt(ek));
   
        // is it realy 3-vector?
        TS_ASSERT_EQUALS(e2.size(),3);
        // is this [0,1,0] ?
        TS_ASSERT_DELTA(e2[1],1,FLT_EPSILON);
        TS_ASSERT_DELTA(e2[0],0,FLT_EPSILON);
        TS_ASSERT_DELTA(e2[2],0,FLT_EPSILON);

        // this is 1-vector of 4-th dimension
        std::vector<double> e4;
        TS_ASSERT_THROWS_NOTHING(e4=space.getOrt(en));
        // is this realy a 1-vector?
        TS_ASSERT_EQUALS(e4.size(),1);
        // is this 1?
        TS_ASSERT_DELTA(e4[0],1,FLT_EPSILON);


        // check if we are getting proper numbers for id-s
        TS_ASSERT_EQUALS((space.getDimRefNum(eh)),0);
        // this dimension does not exis in 5D workspace
        TS_ASSERT_EQUALS(space.getDimRefNum(u7,true),-1);
        TS_ASSERT_THROWS_ANYTHING(space.getDimRefNum(u7));


        unsigned int nDim;
        TS_ASSERT_THROWS_NOTHING(nDim=space.getDimRefNum(ek));

        // let's try to kill old geometry and build a new one;
        DimensionsID iID[]={en,u1,u2,u3};
        std::vector<DimensionsID> ID(iID,iID+4);

        //would not work without any reciprocal dimension, one has to be present;
       TS_ASSERT_THROWS_ANYTHING(space.reinit_WorkspaceGeometry(ID));

        DimensionsID  iID2[]={eh,en,u1,u2,u3,ek};
        ID.assign(iID2,iID2+6);

        // should initiate 2D+4 geometry and all dimensions sorted properly
        TS_ASSERT_THROWS_NOTHING(space.reinit_WorkspaceGeometry(ID));
       // WorkspaceGeometry dimensions are arranged according to growth,
        TS_ASSERT_EQUALS(space.getDimRefNum(eh),0);
        TS_ASSERT_EQUALS(space.getDimRefNum(ek),1);
        TS_ASSERT_EQUALS(space.getDimRefNum(en),2);
        TS_ASSERT_EQUALS(space.getDimRefNum(u1),3);
        TS_ASSERT_EQUALS(space.getDimRefNum(u2),4);
        TS_ASSERT_EQUALS(space.getDimRefNum(u3),5);


/// the techncalities of woriking with 2D+1 and 1D+1 workspaced have not been covered 

    }
};



#endif