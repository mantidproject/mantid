#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

using namespace Mantid;
using namespace Geometry;

// class accessor to protected properties;
class tMDGeometry: public MDGeometry
{
public:
    tMDGeometry(unsigned int nDims=4):MDGeometry(nDims){};
    void setRanges(MDGeometryDescription &slice){MDGeometry::setRanges(slice);}
    void reinit_Geometry(const MDGeometryDescription &slice){MDGeometry::reinit_Geometry(slice);}

};

class testMDGeometry : public CxxTest::TestSuite
{
    tMDGeometry *tDND_geometry;
    MDGeometryDescription *pSlice;
public:
    void testGeometryConstr(void)
    {
        TS_ASSERT_THROWS_NOTHING(tDND_geometry= new tMDGeometry());
    }
    void testMDGeometryDimAccessors(void){
          TS_ASSERT_THROWS_NOTHING(tDND_geometry->getXDimension());
          TS_ASSERT_THROWS_NOTHING(tDND_geometry->getYDimension());
          TS_ASSERT_THROWS_NOTHING(tDND_geometry->getZDimension());
          TS_ASSERT_THROWS_NOTHING(tDND_geometry->getTDimension());

    }
    void testMDGeomIntegrated(void){
          std::vector<MDDimension *> Dims;
          TS_ASSERT_THROWS_NOTHING(Dims=tDND_geometry->getIntegratedDimensions());
          // default size of the dimensions is equal 4
          TS_ASSERT_EQUALS(Dims.size(),4);
    }
    void testMDGeomDimAcessors(void){
    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. 
        MDDimension *pDim;
        // get pointer to the dimension 0
        TS_ASSERT_THROWS_NOTHING(pDim=tDND_geometry->getDimension(0));
        TS_ASSERT_EQUALS(pDim->getDimensionTag(),"q1");
        MDDimension *pDim0;
        // no such dimension
        TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension(8));
        // no such dimension
        TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension("u7"));
//        TS_ASSERT_EQUALS(pDim0,NULL);

        // the same dimension as above
        TS_ASSERT_THROWS_NOTHING(pDim0=tDND_geometry->getDimension("q1"));
        TS_ASSERT_EQUALS(pDim0,pDim);
    }
    void testSlicingProperty(void){
         TS_ASSERT_THROWS_NOTHING(pSlice = new MDGeometryDescription(*tDND_geometry));

//       we want these data to be non-integrated;
         TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins("en",100));
         // wrong tag
         TS_ASSERT_THROWS_ANYTHING(pSlice->setNumBins("eh",200));
         // right tag
         TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins("q1",200));

// we want first (0) axis to be energy 
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"en"));
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"en"));
// and the third (2) ->el (z-axis) 
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"q3"));
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(2,"q3"));
        
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"q1"));

         std::vector<std::string> names = pSlice->getDimensionsTags();
         for(unsigned int i=0;i<names.size();i++){
             TS_ASSERT_EQUALS(names[i],pSlice->getTag(i));
             TS_ASSERT_EQUALS(names[i],pSlice->getAxisName(i));
         }

    }
    void testMDGeomSetFromSlice1(void){
         TS_ASSERT_THROWS_NOTHING(tDND_geometry->setRanges(*pSlice));
         unsigned int i,ic;
           
         MDDimension *pDim;
    
         std::vector<std::string> expanded_tags(tDND_geometry->getNumDims());

         // arrange dimensions tags like the dimensions are arranged in the geometry
         ic=0;
         TS_ASSERT_THROWS_NOTHING(
         for(i=0;i<expanded_tags.size();i++){
           if(pSlice->numBins(i)>1){  // non-integrated;
             expanded_tags[ic]=pSlice->getTag(i);
             ic++;
           }
         }
         for(i=0;i<expanded_tags.size();i++){
           if(pSlice->numBins(i)<2){  // non-integrated;
             expanded_tags[ic]=pSlice->getTag(i);
             ic++;
           }
         }
         )



         for(i=0;i<tDND_geometry->getNumDims();i++){
                   TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(i));
                   TS_ASSERT_EQUALS(pDim->getDimensionTag(),expanded_tags[i]);
         }

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(0));
         TS_ASSERT_EQUALS(pDim->getStride(),1);

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(1));
         TS_ASSERT_EQUALS(pDim->getStride(),100);
         TS_ASSERT_EQUALS(pDim->getIntegrated(),false);

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(2));
         TS_ASSERT_EQUALS(pDim->getStride(),0);
         TS_ASSERT_EQUALS(pDim->getIntegrated(),true);
    }
   void testMDGeomSetFromSlice2(void){
        // this should be fully equivalent to 1

         // arrange final dimensions according to pAxis, this will run through one branch of reinit_Geometry only
         TS_ASSERT_THROWS_NOTHING(tDND_geometry->reinit_Geometry(*pSlice));
         
         MDDimension *pDim;


         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(0));
         TS_ASSERT_EQUALS(pDim->getStride(),1);

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(1));
         TS_ASSERT_EQUALS(pDim->getStride(),100);
         TS_ASSERT_EQUALS(pDim->getIntegrated(),false);

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(2));
         TS_ASSERT_EQUALS(pDim->getStride(),0);
         TS_ASSERT_EQUALS(pDim->getIntegrated(),true);
    }
    

    testMDGeometry(void):tDND_geometry(NULL),pSlice(NULL){};
    ~testMDGeometry(void){
        if(tDND_geometry){
            delete tDND_geometry;
            tDND_geometry=NULL;
        }
        if(pSlice){
            delete pSlice;
            pSlice=NULL;
        }
    }

};
