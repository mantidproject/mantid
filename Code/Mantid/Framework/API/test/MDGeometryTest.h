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
    IMDDimension_sptr dim(new MDHistoDimension("Qx", "Qx", "Ang", -1, +1, 0));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", "Ang", -1, +1, 0));
    dims.push_back(dim);
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


};


#endif /* MANTID_API_MDGEOMETRYTEST_H_ */

