#ifndef H_TEST_WORKSPACE_GEOMETRY
#define H_TEST_WORKSPACE_GEOMETRY

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include <cfloat>

using namespace Mantid;
using namespace Geometry;


class testWorkspaceGm :   public CxxTest::TestSuite
{
    MDGeometryBasis *pGeometry5x3;
    MDGeometryBasis *pGeom4x2;
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
      TS_ASSERT_THROWS_ANYTHING(pGeom4x2= new MDGeometryBasis(non_default_tags,2));

      // now should be ok;
      non_default_tags[2]="cc";
      TS_ASSERT_THROWS_NOTHING(pGeom4x2= new MDGeometryBasis(non_default_tags,2));
        // get workspace name;
        std::string ws_name;
        TS_ASSERT_THROWS_NOTHING(ws_name=pGeom4x2->getWorkspaceIDname());
        TS_ASSERT_EQUALS(ws_name,"aa:bb:cc:dddd:_NDIM_4x2");
    }
    void testWorkspaceGeometryConstructor(void){

        // we can not define such dimensions
        TS_ASSERT_THROWS_ANYTHING(MDGeometryBasis  space1(-1));

        TS_ASSERT_THROWS_ANYTHING(MDGeometryBasis  space1(22));
        TS_ASSERT_THROWS_ANYTHING(MDGeometryBasis  space1(4,4));
        TS_ASSERT_THROWS_ANYTHING(MDGeometryBasis  space1(5,4));
        // the geometry which you can not initiate number of real dimensions lower than number of reciprocal dimensions
        TS_ASSERT_THROWS_ANYTHING(MDGeometryBasis space(2));

        // now we do define 5-d workspace
        TS_ASSERT_THROWS_NOTHING(pGeometry5x3 = new MDGeometryBasis(5));
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

    testWorkspaceGm():pGeometry5x3(NULL),pGeom4x2(NULL){}
    ~testWorkspaceGm(){
        if( pGeometry5x3){     delete pGeometry5x3;
        }
        if (pGeom4x2){      delete pGeom4x2;
        }
    }
};



#endif
