#ifndef _TEST_MD_GEOMETRY_H
#define _TEST_MD_GEOMETRY_H

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

using namespace Mantid::Geometry;


class testMulitDimensionalGeometry : public CxxTest::TestSuite
{
  std::auto_ptr<MDGeometry> tDND_geometry;
  std::auto_ptr<MDGeometryDescription> pSlice;
public:

  testMulitDimensionalGeometry()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 1));
    basisDimensions.insert(MDBasisDimension("qy", true, 2));
    basisDimensions.insert(MDBasisDimension("qz", true, 4));
    basisDimensions.insert(MDBasisDimension("p", false, 0));

    UnitCell cell;
    tDND_geometry= std::auto_ptr<MDGeometry>(new MDGeometry(MDGeometryBasis(basisDimensions, cell)));
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
    TS_ASSERT_EQUALS(pDim->getDimensionTag(),"qx");
    MDDimension *pDim0;
    // no such dimension
    TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension(8));
    // no such dimension
    TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension("u7"));
    //        TS_ASSERT_EQUALS(pDim0,NULL);

    // the same dimension as above
    TS_ASSERT_THROWS_NOTHING(pDim0=tDND_geometry->getDimension("qx"));
    TS_ASSERT_EQUALS(pDim0,pDim);
  }
  void testSlicingProperty(void){
    pSlice = std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription(*tDND_geometry));

    //       we want these data to be non-integrated;
    TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins("p",100));
    // wrong tag
    TS_ASSERT_THROWS_ANYTHING(pSlice->setNumBins("eh",200));
    // right tag
    TS_ASSERT_THROWS_NOTHING(pSlice->setNumBins("qx",200));

    // we want first (0) axis to be energy 
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"p"));
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"p"));
    // and the third (2) ->el (z-axis) 
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"qz"));
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(2,"qz"));

    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"qx"));

    std::vector<std::string> names = pSlice->getDimensionsTags();
    for(unsigned int i=0;i<names.size();i++){
      TS_ASSERT_EQUALS(names[i],pSlice->getTag(i));
      TS_ASSERT_EQUALS(names[i],pSlice->getAxisName(i));
    }

  }
  void testMDGeomSetFromSlice1(void){
   
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->reinit_Geometry(*pSlice));
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

  void testgetNumDims()
  {
     TSM_ASSERT_EQUALS("The number of dimensions returned is not equal to the expected value.", 4, tDND_geometry->getNumDims());
  }
  /// returns the number of reciprocal dimensions
  void  testGetNumReciprocalDims()
  {
   
    TSM_ASSERT_EQUALS("The number of reciprocal dimensions returned is not equal to the expected value.", 3, tDND_geometry->getNumReciprocalDims());
  }

  void testGetNumExpandedDims()
  {
    TSM_ASSERT_EQUALS("The number of expanded dimensions returned is not equal to the expected value.", 2, tDND_geometry->getNumExpandedDims());
  }

};
#endif