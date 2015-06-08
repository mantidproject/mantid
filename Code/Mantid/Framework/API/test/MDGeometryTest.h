#ifndef MANTID_API_MDGEOMETRYTEST_H_
#define MANTID_API_MDGEOMETRYTEST_H_

#include "MantidAPI/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/VMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidAPI/NullCoordTransform.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class MDGeometryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDGeometryTest *createSuite() { return new MDGeometryTest(); }
  static void destroySuite( MDGeometryTest *suite ) { delete suite; }

  void test_initGeometry()
  {
    MDGeometry g;
    std::vector<IMDDimension_sptr> dims;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", "Ang", -1, +1, 20));
    dims.push_back(dim1);
    dims.push_back(dim2);
    g.initGeometry(dims);

    TS_ASSERT_EQUALS( g.getNumDims(), 2);
    TS_ASSERT_EQUALS( g.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS( g.getDimension(1)->getName(), "Qy");
    // Now set the basis vectors
    g.setBasisVector(0, VMD(1.2, 3.4));
    g.setBasisVector(1, VMD(1.2, 3.4));
    // Out of bounds
    TS_ASSERT_THROWS_ANYTHING( g.setBasisVector(2, VMD(1.2, 3.4)) );
    TS_ASSERT_EQUALS( g.getBasisVector(0), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS( g.getBasisVector(1), VMD(1.2, 3.4));

    // Get the resolution
    std::vector<coord_t> binSizes = g.estimateResolution();
    TS_ASSERT_EQUALS( binSizes.size(), 2);
    TS_ASSERT_DELTA( binSizes[0], 0.2, 1e-6);
    TS_ASSERT_DELTA( binSizes[1], 0.1, 1e-6);
  }

  void test_clear_transforms_to_original()
  {
      MDGeometry geometry;
      geometry.setTransformToOriginal(new NullCoordTransform, 0);
      geometry.setTransformToOriginal(new NullCoordTransform, 1);
      TS_ASSERT_EQUALS(2, geometry.getNumberTransformsToOriginal());
      TS_ASSERT_THROWS_NOTHING(geometry.clearTransforms());
      TSM_ASSERT_EQUALS("Should have no transforms", 0, geometry.getNumberTransformsToOriginal());
  }

  void test_clear_transforms_from_original()
  {
      MDGeometry geometry;
      geometry.setTransformFromOriginal(new NullCoordTransform, 0);
      geometry.setTransformFromOriginal(new NullCoordTransform, 1);
      TS_ASSERT_EQUALS(2, geometry.getNumberTransformsFromOriginal());
      TS_ASSERT_THROWS_NOTHING(geometry.clearTransforms());
      TSM_ASSERT_EQUALS("Should have no transforms", 0, geometry.getNumberTransformsFromOriginal());
  }

  void test_clear_original_workspaces()
  {
      MDGeometry geometry;
      boost::shared_ptr<WorkspaceTester> ws0(new WorkspaceTester());
      boost::shared_ptr<WorkspaceTester> ws1(new WorkspaceTester());
      geometry.setOriginalWorkspace(ws0, 0);
      geometry.setOriginalWorkspace(ws1, 1);
      TS_ASSERT_EQUALS(2, geometry.numOriginalWorkspaces());
      TS_ASSERT_THROWS_NOTHING(geometry.clearOriginalWorkspaces());
      TS_ASSERT_EQUALS(0, geometry.numOriginalWorkspaces());
  }

  void test_copy_constructor()
  {
    MDGeometry g;
    std::vector<IMDDimension_sptr> dims;
    IMDDimension_sptr dim0(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 0));
    IMDDimension_sptr dim1(new MDHistoDimension("Qy", "Qy", "Ang", -1, +1, 0));
    dims.push_back(dim0);
    dims.push_back(dim1);
    g.initGeometry(dims);
    g.setBasisVector(0, VMD(1.2, 3.4));
    g.setBasisVector(1, VMD(1.2, 3.4));
    g.setOrigin(VMD(4,5));
    boost::shared_ptr<WorkspaceTester> ws0(new WorkspaceTester());
    boost::shared_ptr<WorkspaceTester> ws1(new WorkspaceTester());
    g.setOriginalWorkspace(ws0, 0);
    g.setOriginalWorkspace(ws1, 1);
    g.setTransformFromOriginal(new NullCoordTransform(5), 0);
    g.setTransformFromOriginal(new NullCoordTransform(6), 1);
    g.setTransformToOriginal(new NullCoordTransform(7), 0);
    g.setTransformToOriginal(new NullCoordTransform(8), 1);

    // Perform the copy
    MDGeometry g2(g);

    TS_ASSERT_EQUALS( g2.getNumDims(), 2);
    TS_ASSERT_EQUALS( g2.getBasisVector(0), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS( g2.getBasisVector(1), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS( g2.getOrigin(), VMD(4,5));
    TS_ASSERT_EQUALS( g2.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS( g2.getDimension(1)->getName(), "Qy");
    // Dimensions are deep copies
    TS_ASSERT_DIFFERS( g2.getDimension(0), dim0);
    TS_ASSERT_DIFFERS( g2.getDimension(1), dim1);
    // Workspaces are not deep-copied, just references
    TS_ASSERT_EQUALS( g2.getOriginalWorkspace(0), ws0);
    TS_ASSERT_EQUALS( g2.getOriginalWorkspace(1), ws1);
    // But transforms are deep-copied
    TS_ASSERT_DIFFERS( g2.getTransformFromOriginal(0), g.getTransformFromOriginal(0));
    TS_ASSERT_DIFFERS( g2.getTransformFromOriginal(1), g.getTransformFromOriginal(1));
    TS_ASSERT_DIFFERS( g2.getTransformToOriginal(0), g.getTransformToOriginal(0));
    TS_ASSERT_DIFFERS( g2.getTransformToOriginal(1), g.getTransformToOriginal(1));
    TS_ASSERT( g2.getTransformFromOriginal(0) != NULL);
    TS_ASSERT( g2.getTransformFromOriginal(1) != NULL);
    TS_ASSERT( g2.getTransformToOriginal(0) != NULL);
    TS_ASSERT( g2.getTransformToOriginal(1) != NULL);

  }


  /** Adding dimension info and searching for it back */
  void test_addDimension_getDimension()
  {
    MDGeometry g;
    MDHistoDimension_sptr dim(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING( g.addDimension(dim); )
    MDHistoDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", "Ang", -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING( g.addDimension(dim2); )
    TS_ASSERT_EQUALS( g.getNumDims(), 2);
    TS_ASSERT_EQUALS( g.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS( g.getDimension(1)->getName(), "Qy");
    TS_ASSERT_EQUALS( g.getDimensionIndexByName("Qx"), 0);
    TS_ASSERT_EQUALS( g.getDimensionIndexByName("Qy"), 1);
    TS_ASSERT_THROWS_ANYTHING( g.getDimensionIndexByName("IDontExist"));
  }

  void test_transformDimensions()
  {
    MDGeometry g;
    MDHistoDimension_sptr dim(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING( g.addDimension(dim); )
    MDHistoDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", "Ang", -2, +2, 0));
    TS_ASSERT_THROWS_NOTHING( g.addDimension(dim2); )
    TS_ASSERT_EQUALS( g.getNumDims(), 2);
    boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
    g.setOriginalWorkspace(ws);
    TS_ASSERT(g.hasOriginalWorkspace());

    // Now transform
    std::vector<double> scaling(2);
    std::vector<double> offset(2);
    scaling[0] = 2.0; scaling[1] = 4.0;
    offset[0] = 0.5; offset[1] = -3;
    g.transformDimensions(scaling, offset);

    // resulting workspace
    TSM_ASSERT("Clear the original workspace", !g.hasOriginalWorkspace());
    TS_ASSERT_EQUALS( g.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS( g.getDimension(1)->getName(), "Qy");
    TS_ASSERT_DELTA( g.getDimension(0)->getMinimum(), -1.5, 1e-4);
    TS_ASSERT_DELTA( g.getDimension(0)->getMaximum(), +2.5, 1e-4);
    TS_ASSERT_DELTA( g.getDimension(1)->getMinimum(), -11., 1e-4);
    TS_ASSERT_DELTA( g.getDimension(1)->getMaximum(), +5.,  1e-4);

    // Bad size throws
    scaling.push_back(123);
    TS_ASSERT_THROWS_ANYTHING( g.transformDimensions(scaling, offset) );
  }

  void test_origin()
  {
    MDGeometry g;
    g.setOrigin(VMD(1.2, 3.4));
    TS_ASSERT_EQUALS( g.getOrigin(), VMD(1.2, 3.4));
  }

  void test_OriginalWorkspace()
  {
    MDGeometry g;
    TS_ASSERT(!g.hasOriginalWorkspace());
    boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
    g.setOriginalWorkspace(ws);
    TS_ASSERT(g.hasOriginalWorkspace());
  }

  void test_OriginalWorkspace_multiple()
  {
    MDGeometry g;
    TS_ASSERT(!g.hasOriginalWorkspace());
    boost::shared_ptr<WorkspaceTester> ws0(new WorkspaceTester());
    boost::shared_ptr<WorkspaceTester> ws1(new WorkspaceTester());
    g.setOriginalWorkspace(ws0);
    g.setOriginalWorkspace(ws1, 1);
    TS_ASSERT(g.hasOriginalWorkspace());
    TS_ASSERT(g.hasOriginalWorkspace(1));
    TS_ASSERT_EQUALS(g.numOriginalWorkspaces(), 2);
  }

  /** If a MDGeometry workspace holds a pointer to an original workspace
   * that gets deleted, remove the pointer and allow it to be destructed.
   */
  void test_OriginalWorkspace_gets_deleted()
  {
    MDGeometry g;
    {
      boost::shared_ptr<WorkspaceTester> ws(new WorkspaceTester());
      AnalysisDataService::Instance().addOrReplace("MDGeometryTest_originalWS", ws);
      g.setOriginalWorkspace(ws);
      TS_ASSERT(g.hasOriginalWorkspace());
    }
    // Workspace is still valid even if it went out of scope
    TS_ASSERT(g.getOriginalWorkspace())

    // Create a different workspace and delete that
    boost::shared_ptr<WorkspaceTester> ws2(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("MDGeometryTest_some_other_ws", ws2);
    AnalysisDataService::Instance().remove("MDGeometryTest_some_other_ws");
    TSM_ASSERT("Different workspace does not get deleted incorrectly", g.hasOriginalWorkspace())

    // Delete the right workspace (e.g. DeleteWorkspace algo)
    AnalysisDataService::Instance().remove("MDGeometryTest_originalWS");
    TSM_ASSERT("Original workspace reference was deleted.", !g.hasOriginalWorkspace());
    TSM_ASSERT("Original workspace reference is cleared.", !g.getOriginalWorkspace());
  }

  void test_transforms_to_original()
  {
    MDGeometry g;
    g.setTransformFromOriginal(new NullCoordTransform, 0);
    g.setTransformFromOriginal(new NullCoordTransform, 1);
    TSM_ASSERT_EQUALS("Wrong number of transforms from original reported.", 2, g.getNumberTransformsFromOriginal());
  }

  void test_transforms_from_original()
  {
    MDGeometry g;
    g.setTransformToOriginal(new NullCoordTransform, 0);
    g.setTransformToOriginal(new NullCoordTransform, 1);
    TSM_ASSERT_EQUALS("Wrong number of transforms to original reported.", 2, g.getNumberTransformsToOriginal());
  }


};


#endif /* MANTID_API_MDGEOMETRYTEST_H_ */

