#ifndef H_TEST_WORKSPACE_GEOMETRY
#define H_TEST_WORKSPACE_GEOMETRY

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <cfloat>

using namespace Mantid;
using namespace Geometry;

class publicWorkspaceGeometry: public MDGeometryBasis
{
public:
    publicWorkspaceGeometry(unsigned int nDims=4,unsigned int nReciprocalDims=3):MDGeometryBasis(nDims,nReciprocalDims){};
    publicWorkspaceGeometry(const std::vector<std::string> &tags,unsigned int nReciprocalDims=3):MDGeometryBasis(tags,nReciprocalDims){};
    int getDimNum(const std::string &tag, bool nothrow=false)const{return MDGeometryBasis::getDimNum(tag,nothrow);}
    std::string getColumnName(unsigned int nColumn)const{return MDGeometryBasis::getColumnName(nColumn);}
    void reinit_GeometryBasis(const std::vector<std::string> &ID,unsigned int nReciprocal_dims=3){MDGeometryBasis::reinit_GeometryBasis(ID,nReciprocal_dims);}
      /// function checks if the tags supplied  coinside with the tags for current basis e.g all existing tags have to be here (the order of tags may be different)
    bool checkTagsCompartibility(const std::vector<std::string> &newTags)const{return MDGeometryBasis::checkTagsCompartibility(newTags);}
    std::vector<int>  getColumnNumbers(const std::vector<std::string> &tag_list)const{return   MDGeometryBasis::getColumnNumbers(tag_list);}
    std::vector<MDGeometryBasis::DimensionID> getDimIDs(void)const{return MDGeometryBasis::getDimIDs();}

};

class testWorkspaceGm :   public CxxTest::TestSuite
{
    publicWorkspaceGeometry *pGeometry5x3;
    publicWorkspaceGeometry *pGeom4x2;
    std::vector<std::string> names;
    std::vector<std::string> default_tags;
 
public:
    void test2x4Basis(){
      std::vector<std::string> non_default_tags(4,"");
      non_default_tags[0]="aa";
      non_default_tags[1]="bb";
      non_default_tags[2]="bb";
      non_default_tags[3]="dddd";
      // equivalent tags would not initiate
      TS_ASSERT_THROWS_ANYTHING(pGeom4x2= new publicWorkspaceGeometry(non_default_tags,2));

      // now should be ok;
      non_default_tags[2]="cc";
      TS_ASSERT_THROWS_NOTHING(pGeom4x2= new publicWorkspaceGeometry(non_default_tags,2));
        // get workspace name;
        std::string ws_name;
        TS_ASSERT_THROWS_NOTHING(ws_name=pGeom4x2->getWorkspaceIDname());
        TS_ASSERT_EQUALS(ws_name,"aa:bb:cc:dddd:_NDIM_4x2");
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
        std::string default_name("q1:q2:q3:en:u1:_NDIM_5x3");
        TS_ASSERT_EQUALS(pGeometry5x3->getWorkspaceIDname(),default_name);

        TS_ASSERT_THROWS_NOTHING(default_tags=pGeometry5x3->getBasisTags());
   
    }

    void testDIMIDS(){
         if(!pGeom4x2)TS_FAIL("MDGeomBasis class has not been constructed");
         if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

         std::vector<MDGeometryBasis::DimensionID> dimIDs;


         TS_ASSERT_THROWS_NOTHING(dimIDs=pGeometry5x3->getDimIDs());
         for(unsigned int i=0;i<dimIDs.size();i++){
           TS_ASSERT_EQUALS(dimIDs[i].getDimensionTag(),default_tags[i]);
           if(i<3){
             TS_ASSERT_EQUALS(dimIDs[i].isReciprocal(),true);
           }else{
             TS_ASSERT_EQUALS(dimIDs[i].isReciprocal(),false);
           }
         }

    }
    void testColNumbers(void){
        std::vector<std::string> tags(3,"");
        tags[0]="aa";
        tags[1]="bb";
        tags[2]="dddd";
        if(!pGeom4x2)TS_FAIL("MDGeomBasis class has not been constructed");


        std::vector<int> nums;
        TS_ASSERT_THROWS_NOTHING(nums = pGeom4x2->getColumnNumbers(tags));

        TS_ASSERT_EQUALS(nums[0],0);
        TS_ASSERT_EQUALS(nums[1],1);
        TS_ASSERT_EQUALS(nums[2],3);

    }
    void testTagsCompartibility(){
      if(!pGeom4x2)TS_FAIL("MDGeomBasis class has not been constructed");
      std::vector<std::string> new_tags(4,"");
      new_tags[0]="cc";
      new_tags[1]="bb";
      new_tags[2]="aa";
      new_tags[3]="dddd";
      TS_ASSERT_EQUALS(pGeom4x2->checkTagsCompartibility(new_tags),true);
      new_tags[0]="q1";
      TS_ASSERT_EQUALS(pGeom4x2->checkTagsCompartibility(new_tags),false);

    }
    void testNDim(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

          // we have defined  5 dimension above
        TS_ASSERT_EQUALS(pGeometry5x3->getNumDims(),5);
        TS_ASSERT_EQUALS(pGeometry5x3->getNumReciprocalDims(),3);
        // and 4x2
        if(!pGeom4x2)TS_FAIL("MDGeomBasis class has not been constructed");
        TS_ASSERT_EQUALS(pGeom4x2->getNumDims(),4);
        TS_ASSERT_EQUALS(pGeom4x2->getNumReciprocalDims(),2);

    }
    void testDefaultNames(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

        TS_ASSERT_THROWS_NOTHING(names=pGeometry5x3->getBasisTags());
        // these tags are:
        const char *defaultTags[]={"q1","q2","q3","en","u1"};
        const char *nondef4x2Tags[]={"aa","bb","cc","dddd"};

        for(int i=0;i<5;i++){
            TS_ASSERT_EQUALS(names[i],defaultTags[i]);
        }
        TS_ASSERT_THROWS_NOTHING(names=pGeom4x2->getBasisTags());
        for(int i=0;i<4;i++){
            TS_ASSERT_EQUALS(names[i],nondef4x2Tags[i]);
        }
    }
    void testSimpleDimensionID(){
      // this class should be usually unavaile to users
        MDGeometryBasis::DimensionID ID1(0,"aa",true);
        MDGeometryBasis::DimensionID ID2(1,"bb",true);

        TS_ASSERT_EQUALS(ID1.getDimNum("bb"),-1);
        TS_ASSERT_EQUALS(ID1.getDimNum("aa"),0);

       TS_ASSERT_EQUALS(ID2.getDimNum("blabla"),-1);
       TS_ASSERT_EQUALS(ID2.getDimNum("x"),-1);
       TS_ASSERT_EQUALS(ID2.getDimNum("bb"),1);
       TS_ASSERT_EQUALS(ID2.getDimensionTag(),"bb");

    }
    void testOrts0(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

      // check if we are getting proper numbers for id-s
        TS_ASSERT_EQUALS((pGeometry5x3->getDimNum("q1")),0);
        TS_ASSERT_EQUALS((pGeometry5x3->getDimNum("u1")),4);
        TS_ASSERT_EQUALS((pGeometry5x3->getDimNum("q3")),2);
        // this dimension does not exis in 5D workspace
        TS_ASSERT_EQUALS(pGeometry5x3->getDimNum("u7",false),-1);
        TS_ASSERT_THROWS_ANYTHING(pGeometry5x3->getDimNum("u7",true));
        // and what about column names
        TS_ASSERT_EQUALS((pGeometry5x3->getColumnName(0)),"q1");
        TS_ASSERT_EQUALS((pGeometry5x3->getColumnName(1)),"q2");
        TS_ASSERT_EQUALS((pGeometry5x3->getColumnName(3)),"en");
        TS_ASSERT_EQUALS((pGeometry5x3->getColumnName(4)),"u1");
/*TO DO: ORTS ARE disabled for the moment
        // attempting to get the coordinate of an non-existing dimension
       TS_ASSERT_THROWS_ANYTHING(pGeometry5x3->getOrt("u7"));

     // this is 3-vector of first dimension
        std::vector<double> e2;
     //   TS_ASSERT_THROWS_NOTHING(e2=pGeometry5x3->getOrt("kc"));

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
*/
    }
    void testReinitBasis(){
        if(!pGeometry5x3)TS_FAIL("MDGeomBasis class has not been constructed");

        // let's try to kill old geometry and build a new one; This option should be availible inside a child class only;
        std::vector<std::string>Tags(4,"");
        // this will initiate the tags for "ereciprocal_energyn" to be a first (0) reciprocal dimension
        Tags[0]="reciprocal energy";Tags[1]="hc_reciprocal";Tags[2]="kc_rec";Tags[3]="lc_orthogonal";

        //would not work without any reciprocal dimension, one has to be present;
       TS_ASSERT_THROWS_NOTHING(pGeometry5x3->reinit_GeometryBasis(Tags));

       // WorkspaceGeometry dimensions are arranged according to their definition,
        TS_ASSERT_EQUALS(pGeometry5x3->getDimNum("reciprocal energy"),0);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimNum("hc_reciprocal"),1);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimNum("kc_rec"),2);
        TS_ASSERT_EQUALS(pGeometry5x3->getDimNum("lc_orthogonal"),3);

        TS_ASSERT_EQUALS((pGeometry5x3->getColumnName(0)),"reciprocal energy");

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
