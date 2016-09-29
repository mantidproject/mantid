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
    Mantid::API::MDLeanGeometry lg;

    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    const std::vector<IMDDimension_sptr> dims{dim, dim, dim, dim};

    lg.initGeometry(dims);
  }

  void test_initGeometryAndBasisVectors() {
    Mantid::API::MDLeanGeometry lg;
    std::vector<IMDDimension_sptr> dims;
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 20));
    dims.push_back(dim1);
    dims.push_back(dim2);
    lg.initGeometry(dims);

    TS_ASSERT_EQUALS(lg.getNumDims(), 2);
    TS_ASSERT_EQUALS(lg.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS(lg.getDimension(1)->getName(), "Qy");
    // Now set the basis vectors
    lg.setBasisVector(0, VMD(1.2, 3.4));
    lg.setBasisVector(1, VMD(1.2, 3.4));
    // Out of bounds
    TS_ASSERT_THROWS_ANYTHING(lg.setBasisVector(2, VMD(1.2, 3.4)));
    TS_ASSERT_EQUALS(lg.getBasisVector(0), VMD(1.2, 3.4));
    TS_ASSERT_EQUALS(lg.getBasisVector(1), VMD(1.2, 3.4));

    Mantid::Kernel::VMD d0 = lg.getBasisVector(0);
    const Mantid::Kernel::VMD constD0 = lg.getBasisVector(0);
    TS_ASSERT_EQUALS(constD0, d0);

    // Get the resolution
    std::vector<coord_t> binSizes = lg.estimateResolution();
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
    MDLeanGeometry lg;
    const Mantid::Geometry::QSample frame;
    MDHistoDimension_sptr dim(
        new MDHistoDimension("Qx", "Qx", frame, -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING(lg.addDimension(dim);)
    MDHistoDimension_sptr dim2(
        new MDHistoDimension("Qy", "Qy", frame, -1, +1, 0));
    TS_ASSERT_THROWS_NOTHING(lg.addDimension(dim2);)
    TS_ASSERT_EQUALS(lg.getNumDims(), 2);
    TS_ASSERT_EQUALS(lg.getDimension(0)->getName(), "Qx");
    TS_ASSERT_EQUALS(lg.getDimension(1)->getName(), "Qy");
  }

  void test_nonIntegratedDimensions() {
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 20));
    const std::vector<IMDDimension_sptr> dims{dim1, dim2};

    MDLeanGeometry lg;

    TS_ASSERT_EQUALS(lg.getNonIntegratedDimensions().size(), 0);

    lg.initGeometry(dims);
    TS_ASSERT_EQUALS(lg.getNonIntegratedDimensions().size(), 2);

    IMDDimension_sptr dim3(new MDHistoDimension("Qz", "Qz", frame, -1, -1, 30));
    TS_ASSERT_THROWS_NOTHING(lg.addDimension(dim3));

    TS_ASSERT_EQUALS(lg.getNonIntegratedDimensions().size(), 3);
  }

  void test_getDimensionIndexes() {
    const Mantid::Geometry::QSample frame;
    IMDDimension_sptr dim1(new MDHistoDimension("Qx", "Qx", frame, -1, +1, 10));
    IMDDimension_sptr dim2(new MDHistoDimension("Qy", "Qy", frame, -1, +1, 20));
    IMDDimension_sptr dim3(new MDHistoDimension("Qz", "Qz", frame, -1, -1, 30));

    const std::vector<IMDDimension_sptr> dims = {dim1, dim2, dim3};

    Mantid::API::MDLeanGeometry lg;
    TS_ASSERT_THROWS(lg.getDimensionIndexByName("fail"), std::runtime_error);
    TS_ASSERT_THROWS(lg.getDimensionIndexById("fail"), std::runtime_error);

    lg.initGeometry(dims);

    TS_ASSERT_EQUALS(lg.getDimensionIndexByName("Qx"), 0);
    TS_ASSERT_EQUALS(lg.getDimensionIndexByName("Qy"), 1);
    TS_ASSERT_EQUALS(lg.getDimensionIndexByName("Qz"), 2);
    TS_ASSERT_THROWS_ANYTHING(lg.getDimensionIndexByName(""));
    TS_ASSERT_THROWS_ANYTHING(lg.getDimensionIndexByName("IDontExist"));

    TS_ASSERT_EQUALS(0, lg.getDimensionIndexById(dim1->getDimensionId()));
    TS_ASSERT_EQUALS(1, lg.getDimensionIndexById(dim2->getDimensionId()));
    TS_ASSERT_EQUALS(2, lg.getDimensionIndexById(dim3->getDimensionId()));
    TS_ASSERT_THROWS(lg.getDimensionIndexById(""), std::runtime_error);
    TS_ASSERT_THROWS(lg.getDimensionIndexById("wrong_id_fail"),
                     std::runtime_error);
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
