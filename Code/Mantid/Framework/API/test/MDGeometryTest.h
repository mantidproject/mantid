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
  }

};


#endif /* MANTID_API_MDGEOMETRYTEST_H_ */

