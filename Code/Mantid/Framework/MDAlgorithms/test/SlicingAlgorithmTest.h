#ifndef MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_
#define MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_

#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <iomanip>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of SlicingAlgorithm for testing */
class SlicingAlgorithmImpl : public SlicingAlgorithm
{
  // Make all the members public so I can test them.
  friend class SlicingAlgorithmTest;
public:
  virtual const std::string name() const { return "SlicingAlgorithmImpl";}
  virtual int version() const { return 1;}
  virtual const std::string category() const { return "Testing";}
  virtual const std::string summary() const { return "Summary of this test."; }
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
  IMDEventWorkspace_sptr ws_names;

  SlicingAlgorithmTest()
  {
    ws = MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    ws1 = MDEventsTestHelper::makeMDEW<1>(5, 0.0, 10.0, 1);
    ws2 = MDEventsTestHelper::makeMDEW<2>(5, 0.0, 10.0, 1);
    ws3 = MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    ws4 = MDEventsTestHelper::makeMDEW<4>(5, 0.0, 10.0, 1);
    ws5 = MDEventsTestHelper::makeMDEW<5>(5, 0.0, 10.0, 1);
    /// Workspace with custom names
    ws_names = MDEventsTestHelper::makeAnyMDEW<MDEvent<3>,3>(3, 0.0, 10.0, 1,
        "", "[%dh,k,l]", "Q%d");

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
    TSM_ASSERT_THROWS_ANYTHING("Name not found in input WS", alg.makeAlignedDimensionFromString("SillyName, 1.0, 9.0, 10"));
    TSM_ASSERT_THROWS_ANYTHING("One entry too many means looking for name 'Axis0, 1.0'",
        alg.makeAlignedDimensionFromString("Axis0, 1.0, 9.0, 10, 222"));
    TSM_ASSERT_THROWS_ANYTHING("One entry too few",
        alg.makeAlignedDimensionFromString("Axis0, 11.0, 9.0"));
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

  /// Dimension name is of style "[x,y,z]". Handle this.
  void test_makeAlignedDimensionFromString_NameWithCommas()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws_names;
    TSM_ASSERT_THROWS_NOTHING("", alg.makeAlignedDimensionFromString("[2h,k,l], 1.0, 9.0, 10"));
    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom.size(), 1);
    TS_ASSERT_EQUALS( alg.m_binDimensions.size(), 1);
    if (alg.m_binDimensions.size() < 1) return;
    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom[0], 2);

    IMDDimension_sptr dim = alg.m_binDimensions[0];
    TS_ASSERT_EQUALS( dim->getName(), "[2h,k,l]");
    TS_ASSERT_EQUALS( dim->getUnits(), "m");
    TS_ASSERT_EQUALS( dim->getNBins(), 10);
    TS_ASSERT_EQUALS( dim->getX(10), 9.0);
  }

  /// Allow the user to specify the dimension ID instead of the name.
  void test_makeAlignedDimensionFromString_SpecifyDimensionID()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws_names;
    TSM_ASSERT_THROWS_NOTHING("", alg.makeAlignedDimensionFromString("Q2 , 1.0, 9.0, 10"));
    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom.size(), 1);
    TS_ASSERT_EQUALS( alg.m_binDimensions.size(), 1);
    if (alg.m_binDimensions.size() < 1) return;
    TS_ASSERT_EQUALS( alg.m_dimensionToBinFrom[0], 2);

    IMDDimension_sptr dim = alg.m_binDimensions[0];
    TS_ASSERT_EQUALS( dim->getName(), "[2h,k,l]");
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
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDim0", name1));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDim1", name2));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDim2", name3));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AlignedDim3", name4));
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
    // Set up data that comes from other properties
    alg.m_minExtents.push_back(-5.0);
    alg.m_maxExtents.push_back(+5.0);
    alg.m_numBins.push_back(20);

    TSM_ASSERT_THROWS_ANYTHING("Blank name", alg.makeBasisVectorFromString(",units,1,2,3"));
    TSM_ASSERT_THROWS_ANYTHING("Too many dims", alg.makeBasisVectorFromString("name,units,1,2,3,4"));
    TSM_ASSERT_THROWS_ANYTHING("Too few dims", alg.makeBasisVectorFromString("name,units,1,2"));

    alg.m_numBins[0] = -10;
    TSM_ASSERT_THROWS_ANYTHING("Invalid # of bins", alg.makeBasisVectorFromString("name,units,1,2"));
    TSM_ASSERT_THROWS_NOTHING("Empty string is OK", alg.makeBasisVectorFromString(""));
    TSM_ASSERT_THROWS_NOTHING("Empty string is OK", alg.makeBasisVectorFromString("   "));
    TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
  }

  void test_makeBasisVectorFromString()
  {
    // Test WITH and WITHOUT basis vector normalization
    for (int normalize=0; normalize<2; normalize++)
    {
      SlicingAlgorithmImpl alg; alg.m_inWS = ws;
      // Set up data that comes from other properties
      alg.m_minExtents.push_back(-5.0);
      alg.m_maxExtents.push_back(+5.0);
      alg.m_numBins.push_back(20);
      alg.m_NormalizeBasisVectors = (normalize > 0);

      TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
      TSM_ASSERT_THROWS_NOTHING("", alg.makeBasisVectorFromString(" name, units  , 1,2,3"));
      TS_ASSERT_EQUALS(alg.m_bases.size(), 1);
      TS_ASSERT_EQUALS(alg.m_binDimensions.size(), 1);
      TS_ASSERT_EQUALS(alg.m_binningScaling.size(), 1);
      TS_ASSERT_EQUALS(alg.m_transformScaling.size(), 1);

      VMD basis(1.,2.,3.);
      if (alg.m_NormalizeBasisVectors)
        basis.normalize();

      TS_ASSERT_EQUALS( alg.m_bases[0], basis );
      IMDDimension_sptr dim = alg.m_binDimensions[0];
      TS_ASSERT_EQUALS( dim->getName(), "name");
      TS_ASSERT_EQUALS( dim->getUnits(), "units");
      TS_ASSERT_EQUALS( dim->getNBins(), 20);
      TS_ASSERT_EQUALS( dim->getMinimum(), -5);
      TS_ASSERT_EQUALS( dim->getMaximum(), +5);
      TS_ASSERT_DELTA( dim->getX(5), -2.5, 1e-5);

      if (alg.m_NormalizeBasisVectors)
      {
        TSM_ASSERT_DELTA("Unit transformation scaling if normalizing", alg.m_transformScaling[0], 1.0, 1e-5);
        TSM_ASSERT_DELTA("A bin ranges from 0-0.5 in OUTPUT, which is 0.5 long in the INPUT, "
            "so the binningScaling is 2.",
            alg.m_binningScaling[0], 2., 1e-5);
      }
      else
      {
        TSM_ASSERT_DELTA("Length sqrt(14) in INPUT = 1.0 in output", alg.m_transformScaling[0], sqrt(1.0/14.0), 1e-5);
        TSM_ASSERT_DELTA("A bin ranges from 0-0.5 in OUTPUT, which is 0.5/sqrt(14) long in the INPUT, "
            "so the binningScaling is 2/sqrt(14)",
            alg.m_binningScaling[0], 2./sqrt(14.0), 1e-5);
      }

    }
  }

  /// Create a basis vector with a dimension with [commas,etc] in the name.
  void test_makeBasisVectorFromString_NameWithCommas()
  {
    SlicingAlgorithmImpl alg; alg.m_inWS = ws;
    // Set up data that comes from other properties
    alg.m_minExtents.push_back(-5.0);
    alg.m_maxExtents.push_back(+5.0);
    alg.m_numBins.push_back(20);
    alg.m_NormalizeBasisVectors = true;

    TS_ASSERT_EQUALS(alg.m_bases.size(), 0);
    TSM_ASSERT_THROWS_NOTHING("", alg.makeBasisVectorFromString("[Dumb,Name], units  , 1,2,3"));
    TS_ASSERT_EQUALS(alg.m_bases.size(), 1);
    TS_ASSERT_EQUALS(alg.m_binDimensions.size(), 1);
    TS_ASSERT_EQUALS(alg.m_binningScaling.size(), 1);
    if (alg.m_bases.size() < 1) return;

    VMD basis(1,2,3);
    basis.normalize();
    TS_ASSERT_DELTA( alg.m_bases[0][0], basis[0], 1e-5 );
    TS_ASSERT_DELTA( alg.m_bases[0][1], basis[1], 1e-5 );
    TS_ASSERT_DELTA( alg.m_bases[0][2], basis[2], 1e-5 );
    IMDDimension_sptr dim = alg.m_binDimensions[0];
    TS_ASSERT_EQUALS( dim->getName(), "[Dumb,Name]");
    TS_ASSERT_EQUALS( dim->getDimensionId(), "[Dumb,Name]");
    TS_ASSERT_EQUALS( dim->getUnits(), "units");
    TS_ASSERT_EQUALS( dim->getNBins(), 20);
    TS_ASSERT_EQUALS( dim->getMinimum(), -5);
    TS_ASSERT_EQUALS( dim->getMaximum(), +5);
  }


  //----------------------------------------------------------------------------
  SlicingAlgorithmImpl * do_createGeneralTransform(IMDEventWorkspace_sptr inWS,
      std::string name1, std::string name2, std::string name3, std::string name4,
      VMD translation,
      std::string extents,
      std::string numBins,
      bool ForceOrthogonal=false,
      bool NormalizeBasisVectors=true)
  {
    SlicingAlgorithmImpl * alg = new SlicingAlgorithmImpl();
    alg->m_inWS = inWS;
    alg->initSlicingProps();
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AxisAligned", "0"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVector0", name1));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVector1", name2));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVector2", name3));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("BasisVector3", name4));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputExtents", extents));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputBins", numBins));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Translation", translation.toString(",")));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("NormalizeBasisVectors", NormalizeBasisVectors));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ForceOrthogonal", ForceOrthogonal));
    alg->createTransform();
    return alg;
  }

  void test_createGeneralTransform_failures()
  {
    TSM_ASSERT_THROWS_ANYTHING("No dimensions given",
        do_createGeneralTransform(ws, "", "", "", "", VMD(1,2,3), "", "") );
    TSM_ASSERT_THROWS_ANYTHING("Bad # of dimensions in translation param",
        do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "", "", "", VMD(1,2,3,4), "0,10", "5") );
    TSM_ASSERT_THROWS_ANYTHING("Too many output dims",
        do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", "x,m,1,0,0, 10.0, 10", VMD(1,2,3,4),
            "0,10,0,10,0,10", "5,5,5") );
    TSM_ASSERT_THROWS_ANYTHING("Bad # of dimensions in the OutputExtents",
        do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "", "", "", VMD(1,2,3), "0,10,0,10", "5") );
    TSM_ASSERT_THROWS_ANYTHING("Bad # of dimensions in the OutputBins",
        do_createGeneralTransform(ws, "x,m,1,0,0, 10.0, 10", "", "", "", VMD(1,2,3), "0,10", "5,5") );
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
            "OutX,m," + baseX.toString(","),
            "OutY,m," + baseY.toString(","),
            "OutZ,m," + baseZ.toString(","),
            "", VMD(1,1,0), "0,10,0,10,0,10", "5,5,5");

    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS( alg->m_translation, VMD(1,1,0));
    TS_ASSERT_EQUALS( alg->m_binDimensions.size(), 3);
    TS_ASSERT_EQUALS( alg->m_bases[0], baseX);
    TS_ASSERT_EQUALS( alg->m_bases[1], baseY);
    TS_ASSERT_EQUALS( alg->m_bases[2], baseZ);

    coord_t in[3] = {3.0, 1.0, 2.6f};
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
            "OutX,m," + baseX.toString(","),
            "OutY,m," + baseY.toString(","),
            "OutZ,m," + baseZ.toString(","),
            "", VMD(0,0,0), "0,10,0,10,0,10", "5,5,5");

    TS_ASSERT_EQUALS( alg->m_bases.size(), 3);
    TS_ASSERT_EQUALS( alg->m_translation, VMD(0,0,0));
    TS_ASSERT_EQUALS( alg->m_binDimensions.size(), 3);
    TS_ASSERT_EQUALS( alg->m_bases[0], baseX);
    TS_ASSERT_EQUALS( alg->m_bases[1], baseY);
    TS_ASSERT_EQUALS( alg->m_bases[2], baseZ);

    coord_t in[3] = {3.0, -1.0, 2.6f};
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
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0",  "OutY,m, 0,1,0,0",
            "OutZ,m, 0,0,1,0",  "", VMD(1,1,1,0),
            "0,10,0,10,0,10", "5,5,5");
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
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0",  "OutY,m, 0,1,0,0",
            "OutZ,m, 0,0,1,0",  "OutE,m, 0,0,0,1", VMD(1,1,1,1),
            "0,10,0,10,0,10,0,10", "5,5,5,5");
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
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0",  "OutY,m, 0,-1,0,0",
            "OutZ,m, 0,0,1,0",  "OutE,m, 0,0,0,1", VMD(1,1,1,1),
            "0,10,0,10,0,10,0,10", "5,5,5,5");
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
        do_createGeneralTransform(ws5, "OutX,m, 1,0,0,0,0",  "OutY,m, 0,1,0,0,0",
            "OutZ,m, 0,0,1,0,0",  "", VMD(1,1,1,0,0),
            "0,10,0,10,0,10", "5,5,5");
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
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0",  "OutY,m, 0,1,0,0",
            "",  "", VMD(1,1,0,0),
            "0,10,0,10", "5,5");
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
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0",  "OutY,m, 0,1,0",
            "",  "", VMD(1,1,0),
            "0,10,0,10", "5,5");
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
        do_createGeneralTransform(ws2, "OutX,m, 1,0",  "OutY,m, 0,1",
            "",  "", VMD(1,1),
            "0,10,0,10", "5,5");
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

  //----------------------------------------------------------------------------
  /** Simple (but general) 2D transform but the edge of space
   * in the output workspace is NOT 0,0.
   * (0,0) in the output = (1,1) in the input.
   * Minimum edge in the output = (-9,  -19) in the input
   * Maximum edge in the output = (+11, +21) in the input
   */
  void test_createGeneralTransform_2D_to_2D_withNonZeroOrigin()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws2, "OutX,m, 2,0",  "OutY,m, 0,3",
            "",  "", VMD(1,1),
            "-10,10, -20,20", "5,5");

    TSM_ASSERT_DELTA("Bins are sized 4 in X",
        alg->m_binningScaling[0], 0.25, 1e-5 );
    TSM_ASSERT_DELTA("Bins are sized 8 in Y",
        alg->m_binningScaling[1], 0.125, 1e-5 );

    TSM_ASSERT_DELTA("Basis vectors were normalized so that output length=input length",
        alg->m_transformScaling[0], 1.0, 1e-5 );
    TSM_ASSERT_DELTA("Basis vectors were normalized so that output length=input length",
        alg->m_transformScaling[1], 1.0, 1e-5 );

    // This input coordinate translates to (+2,-12) as seen in the output
    coord_t in[2] = {3, -11.0};
    coord_t out[2];  VMD outV;

    // The "binning" transform
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(2,out), VMD(3.0, 1.0) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(2,out), VMD(+2, -12) );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(2,in), VMD(3., -11.) );

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT( func->isPointContained(VMD(-8.9, -18.9)) );
    TS_ASSERT( func->isPointContained(VMD(-8.9, 0)) );
    TS_ASSERT( func->isPointContained(VMD(0, -18.9)) );
    TS_ASSERT( func->isPointContained(VMD(10.9, 20.9)) );

    TS_ASSERT( !func->isPointContained(VMD(-9.1, 0)) );
    TS_ASSERT( !func->isPointContained(VMD(0, +21.1)) );
    TS_ASSERT( !func->isPointContained(VMD(+11.1, 0)) );
  }

  //----------------------------------------------------------------------------
  /** Simple (but general) 2D transform but the edge of space
   * in the output workspace is NOT 0,0.
   * Also, the basis vectors are length (2,5)
   * (0,0) in the output = (1,1) in the input.
   * Minimum edge in the output (-10,-20) = (-19,  -99) in the input
   * Maximum edge in the output (+10,+20) = (+21, +101) in the input
   */
  void test_createGeneralTransform_2D_to_2D_withNonZeroOrigin_withScaling()
  {
    SlicingAlgorithmImpl * alg =
        do_createGeneralTransform(ws2, "OutX,m, 2,0",  "OutY,m, 0,5",
            "",  "", VMD(1,1),
            "-10,10, -20,20", "5,5",
            false /*force orthogonal*/,
            false /*normalize basis vectors */ );

    TSM_ASSERT_DELTA("Bins along X are sized 8 in the INPUT dimension",
        alg->m_binningScaling[0], 0.125, 1e-5 );
    TSM_ASSERT_DELTA("Bins along Y are sized 40 in the INPUT dimension)",
        alg->m_binningScaling[1], 1./40., 1e-5 );

    TSM_ASSERT_DELTA("Basis vectors were NOT normalized",
        alg->m_transformScaling[0], 0.5, 1e-5 );
    TSM_ASSERT_DELTA("Basis vectors were NOT normalized",
        alg->m_transformScaling[1], 0.2, 1e-5 );

    // This input coordinate translates to (+2,-12)
    // and then scales to (+1,-2.4) in OUTPUT coords
    coord_t in[2] = {3, -11.0};
    coord_t out[2];  VMD outV;

    // The "binning" transform
    /* You are OUTPUT coordinates (+1,-2.4)
       which is offset by (11, 17.6) from the minimum (-10, -20)
       with bins of size (4,8) (in the OUTPUT dimensions),
       which means the bin coordinate is (11/4, 17.6/8) */
    CoordTransform * trans = alg->m_transform;
    TS_ASSERT(trans);
    trans->apply(in, out);
    TS_ASSERT_EQUALS( VMD(2,out), VMD(11./4., 17.6/8.) );

    // The "real" transform from original
    CoordTransform * transFrom = alg->m_transformFromOriginal;
    TS_ASSERT(transFrom);
    transFrom->apply(in, out);
    TS_ASSERT_EQUALS( VMD(2,out), VMD(+1., -2.4) );

    // The "reverse" transform
    CoordTransform * transTo = alg->m_transformToOriginal;
    TS_ASSERT(transTo);
    transTo->apply(out, in);
    TS_ASSERT_EQUALS( VMD(2,in), VMD(3., -11.) );

    // The implicit function
    MDImplicitFunction * func = alg->getImplicitFunctionForChunk(NULL, NULL);
    TS_ASSERT(func);
    TS_ASSERT_EQUALS( func->getNumPlanes(), 4);
    TS_ASSERT( func->isPointContained(VMD(-18.9, -98.9)) );
    TS_ASSERT( func->isPointContained(VMD(+20.9, 100.9)) );

    TS_ASSERT( !func->isPointContained(VMD(-19.1, 0)) );
    TS_ASSERT( !func->isPointContained(VMD(0, -99.1)) );
    TS_ASSERT( !func->isPointContained(VMD(0, +101.1)) );
    TS_ASSERT( !func->isPointContained(VMD(+21.1, 0)) );
  }

  //----------------------------------------------------------------------------
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
        do_createGeneralTransform(ws2, "OutX,m, 1,0",  "OutY,m, 1,1",  "",  "",
            VMD(0.,0.), "0,10,0,10", "5,5");
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
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0",  "OutY,m, 1,1,0",  "",  "",
            VMD(0.,0.,0.), "0,10,0,10", "5,5");
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
        do_createGeneralTransform(ws4, "OutX,m, 1,0,0,0",  "",  "",  "",
            VMD(1,1,0,0), "0,10", "5");
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
        do_createGeneralTransform(ws3, "OutX,m, 1,0,0",  "",  "",  "",
            VMD(1,1,0), "0,10", "5");
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
        do_createGeneralTransform(ws2, "OutX,m, 1,0",  "",  "",  "",
            VMD(1,1), "0,10", "5");
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
    VMD translation(1); translation[0] = 1.0;
    SlicingAlgorithmImpl * alg = do_createGeneralTransform(ws1, "OutX,m, 1",  "",  "",  "",
        translation, "0,10", "5");
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

