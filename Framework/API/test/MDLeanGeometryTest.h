#ifndef MANTID_API_MDLEANGEOMETRYTEST_H_
#define MANTID_API_MDLEANGEOMETRYTEST_H_

#include "MantidAPI/MDLeanGeometry.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/VMD.h"

#include <cxxtest/TestSuite.h>
// for WorkspaceTester
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::coord_t;
using Mantid::API::MDLeanGeometry;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Kernel::VMD;

class MDLeanGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDLeanGeometryTest *createSuite() { return new MDLeanGeometryTest(); }

  static void destroySuite(MDLeanGeometryTest *suite) { delete suite; }

  void test_initGeometry() {
    Mantid::API::MDLeanGeometry g;
    std::vector<IMDDimension_sptr> dims;
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 20));
    dims.push_back(dim1);
    dims.push_back(dim2);
    g.initGeometry(dims);

    TS_ASSERT_EQUALS(g.getNumDims(), 2);
    TS_ASSERT_EQUALS(g.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS(g.getDimension(1)->getName(), "Qy");
    // Now set the basis vectors
    g.setBasisVector(0, VMD(1.2, 3.4));
    g.setBasisVector(1, VMD(1.2, 3.4));
    // Out of bounds
    TS_ASSERT_THROWS_ANYTHING(g.setBasisVector(2, VMD(1.2, 3.4)));
    TS_ASSERT_EQUALS(g.getBasisVector(0), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS(g.getBasisVector(1), VMD(1.2, 3.4));

    // Get the resolution
    std::vector<coord_t> binSizes = g.estimateResolution();
    TS_ASSERT_EQUALS(binSizes.size(), 2);
    TS_ASSERT_DELTA(binSizes[0], 0.2, 1e-6);
    TS_ASSERT_DELTA(binSizes[1], 0.1, 1e-6);
  }

  void test_copy_constructor() {
    MDLeanGeometry g;
    std::vector<IMDDimension_sptr> dims;
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim0(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 0));
    IMDDimension_sptr dim1(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 0));
    dims.push_back(dim0);
    dims.push_back(dim1);
    g.initGeometry(dims);
    g.setBasisVector(0, VMD(1.2, 3.4));
    g.setBasisVector(1, VMD(1.2, 3.4));
    boost::shared_ptr<WorkspaceTester> ws0 =
        boost::make_shared<WorkspaceTester>();
    boost::shared_ptr<WorkspaceTester> ws1 =
        boost::make_shared<WorkspaceTester>();

    // Perform the copy
    MDLeanGeometry g2(g);

    TS_ASSERT_EQUALS(g2.getNumDims(), 2);
    TS_ASSERT_EQUALS(g2.getBasisVector(0), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS(g2.getBasisVector(1), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS(g2.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS(g2.getDimension(1)->getName(), "Qy");
    // Dimensions are deep copies
    TS_ASSERT_DIFFERS(g2.getDimension(0), dim0);
    TS_ASSERT_DIFFERS(g2.getDimension(1), dim1);
  }

  /** Adding dimension info and searching for it back */
  void test_addDimension_getDimension() {
    MDLeanGeometry g;
    const Mantid::Geometry::QSample frame;
    MDHistoDimension_sptr dim(
        new MDHistoDimension("Qx", "Qx", frame, -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING(g.addDimension(dim);)
    MDHistoDimension_sptr dim2(
        new MDHistoDimension("Qy", "Qy", frame, -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING(g.addDimension(dim2);)
    TS_ASSERT_EQUALS(g.getNumDims(), 2);
    TS_ASSERT_EQUALS(g.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS(g.getDimension(1)->getName(), "Qy");
    TS_ASSERT_EQUALS(g.getDimensionIndexByName("Qx"), 0);
    TS_ASSERT_EQUALS(g.getDimensionIndexByName("Qy"), 1);
    TS_ASSERT_THROWS_ANYTHING(g.getDimensionIndexByName("IDontExist"));
  }

  void test_all_normalized() {
    MDLeanGeometry geometry;
    std::vector<IMDDimension_sptr> dims;
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 20));
    IMDDimension_sptr dim3(new MDHistoDimension("Qz", "Qz", frame, -1, +1, 30));
    dims.push_back(dim1);
    dims.push_back(dim2);
    dims.push_back(dim3);
    geometry.initGeometry(dims);

    // None of the basis vectors are initially normalized
    geometry.setBasisVector(0, VMD(2.0, 0.0, 0.0));
    geometry.setBasisVector(1, VMD(0.0, 3.0, 0.0));
    geometry.setBasisVector(2, VMD(0.0, 0.0, 4.0));
    TSM_ASSERT("Not all basis vectors are normalized",
               !geometry.allBasisNormalized());

    // The first basis vector is now normalized. The others are not.
    geometry.setBasisVector(0, VMD(0.0, 1.0, 0.0));
    TSM_ASSERT("Not all basis vectors are normalized",
               !geometry.allBasisNormalized());

    // The second basis vector is now normalized too. The third not yet.
    geometry.setBasisVector(1, VMD(0.0, 1.0, 0.0));
    TSM_ASSERT("Not all basis vectors are normalized",
               !geometry.allBasisNormalized());

    // Last basis vector now normalized
    geometry.setBasisVector(2, VMD(0.0, 0.0, 1.0));
    TSM_ASSERT("All basis vectors are normalized",
               geometry.allBasisNormalized());
  }
};

#endif /* MANTID_API_MDLEANGEOMETRYTEST_H_ */
