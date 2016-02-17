#ifndef MANTID_API_GEOMETRYINFOFACTORY_TEST_H_
#define MANTID_API_GEOMETRYINFOFACTORY_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/GeometryInfoFactory.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class GeometryInfoFactoryTest : public CxxTest::TestSuite {
public:
  static GeometryInfoFactoryTest *createSuite() {
    return new GeometryInfoFactoryTest();
  }
  static void destroySuite(GeometryInfoFactoryTest *suite) { delete suite; }

  GeometryInfoFactoryTest() {
    workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        1, 1, false);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(GeometryInfoFactory info(*workspace));
  }

  void test_getInstrument() {
    auto info = GeometryInfoFactory(*workspace);
    // This might fail if we get a null instrument, so we test for throw. Since
    // workspace->getInstrument() creates a copy of the instrument there is no
    // point in attempting to verify that the pointer is "correct".
    TS_ASSERT_THROWS_NOTHING(info.getInstrument());
  }

  void test_getSource() {
    auto info = GeometryInfoFactory(*workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSource());
  }

  void test_getSample() {
    auto info = GeometryInfoFactory(*workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSample());
  }

  void test_getSourcePos() {
    auto info = GeometryInfoFactory(*workspace);
    TS_ASSERT_EQUALS(info.getSourcePos(), Kernel::V3D(-20.0, 0.0, 0.0));
  }

  void test_getSamplePos() {
    auto info = GeometryInfoFactory(*workspace);
    TS_ASSERT_EQUALS(info.getSamplePos(), Kernel::V3D(0.0, 0.0, 0.0));
  }

  void test_getL1() {
    auto info = GeometryInfoFactory(*workspace);
    TS_ASSERT_EQUALS(info.getL1(), 20.0);
  }

private:
  Workspace2D_sptr workspace;
};

#endif /* MANTID_API_GEOMETRYINFOFACTORY_TEST_H_ */
