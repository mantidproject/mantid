#ifndef MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_
#define MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/SlicingAlgorithm.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;


//------------------------------------------------------------------------------------------------
/** Concrete declaration of SlicingAlgorithm for testing */
class SlicingAlgorithmImpl : public SlicingAlgorithm
{
  // Make all the members public so I can test them.
  friend class SlicingAlgorithmTest;
public:
  virtual const std::string name() const { return "SlicingAlgorithmImpl";};
  virtual int version() const { return 1;};
  virtual const std::string category() const { return "Testing";}
  void init() {}
  void exec() {}
};


//------------------------------------------------------------------------------------------------
class SlicingAlgorithmTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SlicingAlgorithmTest *createSuite() { return new SlicingAlgorithmTest(); }
  static void destroySuite( SlicingAlgorithmTest *suite ) { delete suite; }

  IMDEventWorkspace_sptr ws;
  IMDEventWorkspace_sptr ws1;
  IMDEventWorkspace_sptr ws2;
  IMDEventWorkspace_sptr ws3;
  IMDEventWorkspace_sptr ws4;
  IMDEventWorkspace_sptr ws5;

  SlicingAlgorithmTest()
  {
    ws = MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    ws1 = MDEventsTestHelper::makeMDEW<1>(5, 0.0, 10.0, 1);
    ws2 = MDEventsTestHelper::makeMDEW<2>(5, 0.0, 10.0, 1);
    ws3 = MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    ws4 = MDEventsTestHelper::makeMDEW<4>(5, 0.0, 10.0, 1);
    ws5 = MDEventsTestHelper::makeMDEW<5>(5, 0.0, 10.0, 1);
  }

  void test_initSlicingProps()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    TSM_ASSERT_THROWS_NOTHING("Can init properties", alg.initSlicingProps());
  }

  // ==============================================================================================
  // ================================= AXIS-ALIGNED SLICES ========================================
  // ==============================================================================================
  void test_makeAlignedDimensionFromString_failures()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    TSM_ASSERT_THROWS_ANYTHING("Blank string", alg.makeAlignedDimensionFromString(""));
    TSM_ASSERT_THROWS_ANYTHING("Blank name", alg.makeAlignedDimensionFromString(", 1.0, 9.0, 10"));
    TSM_ASSERT_THROWS_ANYTHING("Min > max", alg.makeAlignedDimensionFromString("Axis0, 11.0, 9.0, 10"));
    TSM_ASSERT_THROWS_ANYTHING("Name not found in input WS", alg.makeAlignedDimensionFromString("SillyName, 1.0, 9.0, 10"));
  }

  void test_makeAlignedDimensionFromString()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    TSM_ASSERT_THROWS_NOTHING("", alg.makeAlignedDimensionFromString("Axis2, 1.0, 9.0, 10"));
    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom.size(), 1);
    TS_ASSERT_EQUALS( alg.m_binDimensions.size(), 1);

    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom[0], 2);

    IMDDimension_sptr dim = alg.m_binDimensions[0];
    TS_ASSERT_EQUALS( dim->getName(), "Axis2");
    TS_ASSERT_EQUALS( dim->getUnits(), "m");
    TS_ASSERT_EQUALS( dim->getNBins(), 10);
    TS_ASSERT_EQUALS( dim->getX(10), 9.0);
  }

  SlicingAlgorithmImpl * do_createAlignedTransform(std::string name1, std::string name2, std::string name3, std::string name4)
  {
    SlicingAlgorithmImpl * alg = new SlicingAlgorithmImpl();
    alg->m_inWS = ws;
    alg->initSlicingProps();
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AxisAligned", "1"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDimX", name1));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDimY", name2));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDimZ", name3));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDimT", name4));
    alg->createTransform();
    return alg;
  }

  void test_createAlignedTransform_failures()
  {
    TSM_ASSERT_THROWS_ANYTHING("3D to 4D fails",  do_createAlignedTransform("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 3", "Axis3,2.0,6.0, 1") );
    TSM_ASSERT_THROWS_ANYTHING("Don't skip entries in the dimensions",  do_createAlignedTransform("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "Axis3,2.0,6.0, 1") );
    TSM_ASSERT_THROWS_ANYTHING("3D to 0D fails",  do_createAlignedTransform("","", "", ""));
    TSM_ASSERT_THROWS_ANYTHING("Dimension name not found",  do_createAlignedTransform("NotAnAxis, 2.0,8.0, 3","", "", ""));
    TSM_ASSERT_THROWS_ANYTHING("0 bins is bad",  do_createAlignedTransform("Axis0, 2.0,8.0, 0","", "", ""));
  }


  void test_createAlignedTransform()
  {
    SlicingAlgorithmImpl * alg =
        do_createAlignedTransform("Axis0, 2.0,8.0, 6", "Axis1, 2.0,8.0, 3", "Axis2, 2.0,8.0, 3", "");

    TS_ASSERT_EQUALS(alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS(alg->m_binDimensions.size(), 3);

    TS_ASSERT_EQUALS( alg->m_bases[0], VMD(1,0,0) );
    TS_ASSERT_EQUALS( alg->m_bases[1], VMD(0,1,0) );
    TS_ASSERT_EQUALS( alg->m_bases[2], VMD(0,0,1) );

    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[0], 0);
    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[1], 1);
    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[2], 2);

    coord_t in[3] = {2.5, 3.5, 4.5};
    coord_t out[3];  VMD outV;

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(0.5, 0.75, 1.25) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(2.5, 3.5, 4.5) );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(3,in), VMD(2.5, 3.5, 4.5) );
  }

  void test_createAlignedTransform_scrambled()
  {
    SlicingAlgorithmImpl * alg =
        do_createAlignedTransform("Axis2, 2.0,8.0, 3", "Axis0, 2.0,8.0, 6", "Axis1, 2.0,8.0, 3", "");

    TS_ASSERT_EQUALS(alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS(alg->m_binDimensions.size(), 3);

    TS_ASSERT_EQUALS( alg->m_bases[0], VMD(0,0,1) );
    TS_ASSERT_EQUALS( alg->m_bases[1], VMD(1,0,0) );
    TS_ASSERT_EQUALS( alg->m_bases[2], VMD(0,1,0) );

    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[0], 2);
    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[1], 0);
    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[2], 1);

    coord_t in[3] = {2.5, 3.5, 4.5};
    coord_t out[3];  VMD outV;

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(1.25, 0.5, 0.75) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(4.5, 2.5, 3.5) );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(3,in), VMD(2.5, 3.5, 4.5) );
  }


  /** Integrate 2 dimensions so that the output has fewer dimensions */
  void test_createAlignedTransform_integrating()
  {
    SlicingAlgorithmImpl * alg = do_createAlignedTransform("Axis0, 2.0,8.0, 6", "", "", "");
    TS_ASSERT_EQUALS( alg->m_bases.size(), 1);
    TS_ASSERT_EQUALS( alg->m_binDimensions.size(), 1);
    TS_ASSERT_EQUALS( alg->m_bases[0], VMD(1,0,0) );
    TS_ASSERT_EQUALS( alg->m_dimensionToBinFrom[0], 0);

    coord_t in[3] = {2.5, 3.5, 4.5};
    coord_t out[1];

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_DELTA( out[0], 0.5, 1e-5 );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_DELTA( out[0], 2.5, 1e-5 );

    // The "reverse" transform does NOT exist
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo == NULL);
  }

  void test_aligned_ImplicitFunction()
  {
    SlicingAlgorithmImpl * alg =
        do_createAlignedTransform("Axis0, 2.0,8.0, 6", "Axis1, 2.0,8.0, 3", "Axis2, 2.0,8.0, 3", "");
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT( func->isPointContained(VMD(3, 4, 5)) );
    TS_ASSERT( !func->isPointContained(VMD(1.9, 4, 5)) );
    TS_ASSERT( !func->isPointContained(VMD(3.9, 9.2, 6.3)) );
  }

  void test_aligned_ImplicitFunction_chunk()
  {
    SlicingAlgorithmImpl * alg =
        do_createAlignedTransform("Axis0, 2.0,8.0, 6", "Axis1, 2.0,8.0, 6", "Axis2, 2.0,8.0, 6", "");
    /* This defines a chunk implicit function between 3-4 in each axis */
    size_t chunkMin[3] = {1, 1, 1};
    size_t chunkMax[3] = {2, 2, 2};
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(chunkMin, chunkMax);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT( func->isPointContained(VMD(3.5, 3.5, 3.5)) );
    TS_ASSERT( !func->isPointContained(VMD(2.9, 3.5, 3.5)) );
    TS_ASSERT( !func->isPointContained(VMD(3.5, 4.1, 3.5)) );
  }




  // ==============================================================================================
  // ================================= NON-AXIS-ALIGNED SLICES ====================================
  // ==============================================================================================

  void test_makeBasisVectorFromString_failures()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
    TSM_ASSERT_THROWS_ANYTHING("Blank name", alg.makeBasisVectorFromString(",units,1,2,3, 10.0, 5"));
    TSM_ASSERT_THROWS_ANYTHING("Too many dims", alg.makeBasisVectorFromString("name,units,1,2,3,4, 10.0, 5"));
    TSM_ASSERT_THROWS_ANYTHING("Too few dims", alg.makeBasisVectorFromString("name,units,1,2, 10.0, 5"));
    TSM_ASSERT_THROWS_ANYTHING("Invalid # of bins", alg.makeBasisVectorFromString("name,units,1,2, 10.0, -2"));
    TSM_ASSERT_THROWS_ANYTHING("Invalid # of bins", alg.makeBasisVectorFromString("name,units,1,2, 10.0, 0"));
    TSM_ASSERT_THROWS_NOTHING("Empty string is OK", alg.makeBasisVectorFromString(""));
    TSM_ASSERT_THROWS_NOTHING("Empty string is OK", alg.makeBasisVectorFromString("   "));
    TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
  }

  void test_makeBasisVectorFromString()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
    TSM_ASSERT_THROWS_NOTHING("", alg.makeBasisVectorFromString(" name, units  , 1,2,3, 10.0, 5"));
    TS_ASSERT_EQUALS(alg.m_bases.size(), 1);
    TS_ASSERT_EQUALS(alg.m_binDimensions.size(), 1);
    TS_ASSERT_EQUALS(alg.m_scaling.size(), 1);

    VMD basis(1,2,3);
    basis.normalize();
    TS_ASSERT_EQUALS( alg.m_bases[0], basis );
    IMDDimension_sptr dim = alg.m_binDimensions[0];
    TS_ASSERT_EQUALS( dim->getName(), "name");
    TS_ASSERT_EQUALS( dim->getUnits(), "units");
    TS_ASSERT_EQUALS( dim->getNBins(), 5);
    TS_ASSERT_EQUALS( dim->getX(5), 10.0);
  }


  SlicingAlgorithmImpl * do_createGeneralTransform(IMDEventWorkspace_sptr inWS, std::string name1, std::string name2, std::string name3, std::string name4,
      VMD origin, bool ForceOrthogonal=false)
  {
    SlicingAlgorithmImpl * alg = new SlicingAlgorithmImpl();
    alg->m_inWS = inWS;
    alg->initSlicingProps();
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AxisAligned", "0"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVectorX", name1));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVectorY", name2));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVectorZ", name3));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVectorT", name4));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Origin", origin.toString(",")));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ForceOrthogonal", ForceOrthogonal));
    alg->createTransform();
    return alg;
  }

  void test_createGeneralTransform_failures()
  {
    TSM_ASSERT_THROWS_ANYTHING("No dimensions given",  do_createGeneralTransform(ws, "", "", "", "", VMD(1,2,3)) );
    TSM_ASSERT_THROWS_ANYTHING("Bad # of dimensions in origin param",  do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "", "", "", VMD(1,2,3,4)) );
    TSM_ASSERT_THROWS_ANYTHING("Too many output dims",  do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", VMD(1,2,3,4)) );
  }

  void test_createGeneralTransform_3D_to_3D()
  {
    // Build the basis vectors, a 0.1 rad rotation along +Z
    double angle = 0.1;
    VMD baseX(cos(angle), sin(angle), 0.0);
    VMD baseY(-sin(angle), cos(angle), 0.0);
    VMD baseZ(0.0, 0.0, 1.0);

    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws3,
            "OutX,m," + baseX.toString(",") + ",10.0, 5",
            "OutY,m," + baseY.toString(",") + ",10.0, 5",
            "OutZ,m," + baseZ.toString(",") + ",10.0, 5",
            "", VMD(1,1,0));

    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS( alg->m_origin, VMD(1,1,0));
    TS_ASSERT_EQUALS( alg->m_binDimensions.size(), 3);
    TS_ASSERT_EQUALS( alg->m_bases[0], baseX);
    TS_ASSERT_EQUALS( alg->m_bases[1], baseY);
    TS_ASSERT_EQUALS( alg->m_bases[2], baseZ);

    coord_t in[3] = {3.0, 1.0, 2.6};
    coord_t out[3];  VMD outV;

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(cos(angle), -sin(angle), 1.3) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(cos(angle), -sin(angle), 1.3)*2 );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(3,in), VMD(3.0, 1.0, 2.6) );

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT( func->isPointContained(VMD(1.5, 1.5, 2)) );
    TS_ASSERT( func->isPointContained(VMD(5.5, 5.5, 4)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, 1.5, -1)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, 1.5, +11)) );
    TS_ASSERT( !func->isPointContained(VMD(0.5, 1.5, 2)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, 0.5, 2)) );
    TS_ASSERT( !func->isPointContained(VMD(11.5, 1.5, 2)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, 11.5, 2)) );
  }


  /** Build a set of basis vectors that is in left-handed coordinates,
   * by flipping the Y basis vector
   */
  void test_createGeneralTransform_3D_to_3D_LeftHanded()
  {
    // Build the basis vectors, a left-handed coordinate system.
    VMD baseX(1.0, 0.0, 0.0);
    VMD baseY(0.0, -1., 0.0);
    VMD baseZ(0.0, 0.0, 1.0);

    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws3,
            "OutX,m," + baseX.toString(",") + ",10.0, 5",
            "OutY,m," + baseY.toString(",") + ",10.0, 5",
            "OutZ,m," + baseZ.toString(",") + ",10.0, 5",
            "", VMD(0,0,0));

    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS( alg->m_origin, VMD(0,0,0));
    TS_ASSERT_EQUALS( alg->m_binDimensions.size(), 3);
    TS_ASSERT_EQUALS( alg->m_bases[0], baseX);
    TS_ASSERT_EQUALS( alg->m_bases[1], baseY);
    TS_ASSERT_EQUALS( alg->m_bases[2], baseZ);

    coord_t in[3] = {3.0, -1.0, 2.6};
    coord_t out[3];  VMD outV;

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(1.5, 0.5, 1.3) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(3,out), VMD(3.0, 1.0, 2.6) );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(3,in), VMD(3.0, -1.0, 2.6) );

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT(  func->isPointContained(VMD(1.5, -1.5, 2)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, 1.5, 2)) );
    TS_ASSERT(  func->isPointContained(VMD(5.5, -5.5, 4)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, -1.5, -1)) );
    TS_ASSERT( !func->isPointContained(VMD(1.5, -1.5, +11)) );
  }



  void test_createGeneralTransform_4D_to_3D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0, 10.0, 5",  "OutY,m, 0,1,0,0, 10.0, 5",
            "OutZ,m, 0,0,1,0, 10.0, 5",  "", VMD(1,1,1,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT(  func->isPointContained( VMD(1.5, 1.5,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5,  12, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5,  0.5, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1., 2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 0.5,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 11.5, 2, 234) ) );
  }

  void test_createGeneralTransform_4D_to_4D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0, 10.0, 5",  "OutY,m, 0,1,0,0, 10.0, 5",
            "OutZ,m, 0,0,1,0, 10.0, 5",  "OutE,m, 0,0,0,1, 10.0, 5", VMD(1,1,1,1));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 4);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 8);
    TS_ASSERT(  func->isPointContained( VMD(1.5, 1.5, 1.5, 1.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5, 1.5,-1.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5, 1.5,11.5) ) );
  }

  /** 4D "left-handed" coordinate system
   * obtained by flipping the Y basis vector.  */
  void test_createGeneralTransform_4D_to_4D_LeftHanded()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0, 10.0, 5",  "OutY,m, 0,-1,0,0, 10.0, 5",
            "OutZ,m, 0,0,1,0, 10.0, 5",  "OutE,m, 0,0,0,1, 10.0, 5", VMD(1,1,1,1));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 4);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 8);
    TS_ASSERT(  func->isPointContained( VMD(1.5, -1.5, 1.5, 1.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, -1.5, 1.5,-1.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, -1.5, 1.5,11.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5,  1.5, 1.5, 1.5) ) );
  }


  void test_createGeneralTransform_5D_to_3D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws5, "OutX,m, 1,0,0,0,0, 10.0, 5",  "OutY,m, 0,1,0,0,0, 10.0, 5",
            "OutZ,m, 0,0,1,0,0, 10.0, 5",  "", VMD(1,1,1,0,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 6);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5,  2, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5,  12, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 1.5,  0.5, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0,  2, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1., 2, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 0.5,  2, 234, 456) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 11.5, 2, 234, 456) ) );
  }



  void test_createGeneralTransform_4D_to_2D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0, 10.0, 5",  "OutY,m, 0,1,0,0, 10.0, 5",  "",  "", VMD(1,1,0,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 2);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1., 2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 0.5,  2, 234) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 11.5, 2, 234) ) );
  }

  void test_createGeneralTransform_3D_to_2D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0, 10.0, 5",  "OutY,m, 0,1,0, 10.0, 5",  "",  "", VMD(1,1,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 2);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5, 2) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0, 2) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1.0, 2) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 0.5, 2) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 11.5, 2) ) );
  }

  void test_createGeneralTransform_2D_to_2D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws2, "OutX,m, 1,0, 10.0, 5",  "OutY,m, 0,1, 10.0, 5",  "",  "", VMD(1,1));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 2);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1.0) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 0.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(1.5, 11.5) ) );
  }

  /** These non-orthogonal bases define a parallelogram sort of like this but at 45 degrees:
   *
   *    /``````/
   *   /      /
   *  /______/
   *
   */
  void test_createGeneralTransform_2D_to_2D_nonOrthogonal()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws2, "OutX,m, 1,0, 10.0, 5",  "OutY,m, 1,1, 10.0, 5",  "",  "", VMD(0.,0.));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 2);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT(  func->isPointContained( VMD(2., 1.) ) );
    TS_ASSERT( !func->isPointContained( VMD(8., 7.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(0., 1.) ) );
    TS_ASSERT( !func->isPointContained( VMD(5., 6.) ) );    // This point would be contained if using orthogonal bases
    TS_ASSERT(  func->isPointContained( VMD(12., 3.) ) );   // This point would NOT be contained if using orthogonal bases
  }

  void test_createGeneralTransform_3D_to_2D_nonOrthogonal()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0, 10.0, 5",  "OutY,m, 1,1,0, 10.0, 5",  "",  "", VMD(0.,0.,0.));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 2);

    // The implicit function
    MDImplicitFunction * func(NULL);
    TS_ASSERT_THROWS_NOTHING( func = alg->getImplicitFunctionForChunk(NULL, NULL) );
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT(  func->isPointContained( VMD(2., 1.,  0.) ) );
    TS_ASSERT( !func->isPointContained( VMD(8., 7.5, 0.) ) );
    TS_ASSERT( !func->isPointContained( VMD(0., 1.,  0.) ) );
    TS_ASSERT( !func->isPointContained( VMD(5., 6.,  0.) ) );   // This point would be contained if using orthogonal bases
    TS_ASSERT(  func->isPointContained( VMD(12., 3., 0.) ) );   // This point would NOT be contained if using orthogonal bases
  }

  void test_createGeneralTransform_4D_to_1D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0, 10.0, 5",  "",  "",  "", VMD(1,1,0,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 1);

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 2);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5, 2, 345) ) );
    TS_ASSERT( func->isPointContained(  VMD(1.5, -12345.5, +23456, 345) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0, 2, 345) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1.0, 2, 345) ) );
  }

  void test_createGeneralTransform_3D_to_1D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0, 10.0, 5",  "",  "",  "", VMD(1,1,0));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 1);

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 2);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5, 2) ) );
    TS_ASSERT( func->isPointContained(  VMD(1.5, -12345.5, +23456) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0, 2) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1.0, 2) ) );
  }

  void test_createGeneralTransform_2D_to_1D()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws2, "OutX,m, 1,0, 10.0, 5",  "",  "",  "", VMD(1,1));
    TS_ASSERT_EQUALS( alg->m_bases.size(), 1);

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 2);
    TS_ASSERT( func->isPointContained(  VMD(1.5, 1.5) ) );
    TS_ASSERT( func->isPointContained(  VMD(1.5, -12345.5) ) );
    TS_ASSERT( !func->isPointContained( VMD(0.5, 1.0) ) );
    TS_ASSERT( !func->isPointContained( VMD(11.1, -1.0) ) );
  }

  void test_createGeneralTransform_1D_to_1D()
  {
    VMD origin(1); origin[0] = 1.0;
    SlicingAlgorithmImpl * alg = do_createGeneralTransform(ws1, "OutX,m, 1, 10.0, 5",  "",  "",  "", origin);
    TS_ASSERT_EQUALS( alg->m_bases.size(), 1);

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 2);
    VMD point(1);
    point[0] = 1.5;
    TS_ASSERT( func->isPointContained(point) );
    point[0] = 11.5;
    TS_ASSERT(!func->isPointContained(point) );
    point[0] = 0.5;
    TS_ASSERT(!func->isPointContained(point) );
  }
};


#endif /* MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_ */

