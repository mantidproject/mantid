#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDGeometry.h"
#include "MDDataObjects/SlicingData.h"

using namespace Mantid;
using namespace MDDataObjects;

// class accessor to protected properties;
class tMDGeometry: public MDGeometry
{
public:
    tMDGeometry(unsigned int nDims=4):MDGeometry(nDims){};
    void setRanges(const SlicingData &slice){MDGeometry::setRanges(slice);}
    void reinit_Geometry(const SlicingData &slice){MDGeometry::reinit_Geometry(slice);}

};
//
class testMDGeometry : public CxxTest::TestSuite
{
    tMDGeometry *tDND_geometry;
    SlicingData *pSlice;
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
          std::vector<Dimension *> Dims;
          TS_ASSERT_THROWS_NOTHING(Dims=tDND_geometry->getIntegratedDimensions());
          // default size of the dimensions is equal 4
          TS_ASSERT_EQUALS(Dims.size(),4);
    }
    void testMDGeomDimAcessors(void){
    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. 
        Dimension *pDim;
        // get pointer to the dimension 0
        TS_ASSERT_THROWS_NOTHING(pDim=tDND_geometry->getDimension(0));
        TS_ASSERT_EQUALS(pDim->getDimensionID(),eh);
        Dimension *pDim0;
        // no such dimension
        TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension(8));
        // no such dimension
        TS_ASSERT_THROWS_NOTHING(pDim0=tDND_geometry->getDimension(u7));
//        TS_ASSERT_EQUALS(pDim0,NULL);

        // the same dimension as above
        TS_ASSERT_THROWS_NOTHING(pDim0=tDND_geometry->getDimension(eh));
        TS_ASSERT_EQUALS(pDim0,pDim);
    }
    void testSlicingData(void){
         TS_ASSERT_THROWS_NOTHING(pSlice = new SlicingData(*tDND_geometry));

//       we want these data to be non-integrated;
         TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins(en,100));
         TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins(eh,200));

// we want first (0) axis to be energy 
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,en));
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,en));
// and the third (2) ->el (z-axis) 
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,el));
         TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(2,el));

    }
    void testMDGeomSetFromSlice(void){
         TS_ASSERT_THROWS_NOTHING(tDND_geometry->setRanges(*pSlice));

         // arrange final dimensions according to pAxis, this will run through one branch of reinit_Geometry only
         TS_ASSERT_THROWS_NOTHING(tDND_geometry->reinit_Geometry(*pSlice));
         
         Dimension *pDim;

         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(0));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),pSlice->getPAxis(0));
         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(1));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),pSlice->getPAxis(1));
         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(2));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),pSlice->getPAxis(2));
         TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(3));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),pSlice->getPAxis(3));
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
