#ifndef MANTID_API_GEOMETRYINFOFACTORY_TEST_H_
#define MANTID_API_GEOMETRYINFOFACTORY_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidAPI/GeometryInfoFactory.h"

using namespace Mantid;
using namespace Mantid::API;

class GeometryInfoFactoryTest : public CxxTest::TestSuite {
public:
  static GeometryInfoFactoryTest *createSuite() {
    return new GeometryInfoFactoryTest();
  }
  static void destroySuite(GeometryInfoFactoryTest *suite) { delete suite; }

  GeometryInfoFactoryTest() : m_workspace(nullptr) {
    size_t numberOfHistograms = 1;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(GeometryInfoFactory info(m_workspace));
  }

  void test_getInstrument() {
    auto info = GeometryInfoFactory(m_workspace);
    // This might fail if we get a null instrument, so we test for throw. Since
    // workspace->getInstrument() creates a copy of the instrument there is no
    // point in attempting to verify that the pointer is "correct".
    TS_ASSERT_THROWS_NOTHING(info.getInstrument());
  }

  void test_getSource() {
    auto info = GeometryInfoFactory(m_workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSource());
  }

  void test_getSample() {
    auto info = GeometryInfoFactory(m_workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSample());
  }

  void test_getSourcePos() {
    auto info = GeometryInfoFactory(m_workspace);
    TS_ASSERT_EQUALS(info.getSourcePos(), Kernel::V3D(-20.0, 0.0, 0.0));
  }

  void test_getSamplePos() {
    auto info = GeometryInfoFactory(m_workspace);
    TS_ASSERT_EQUALS(info.getSamplePos(), Kernel::V3D(0.0, 0.0, 0.0));
  }

  void test_getL1() {
    auto info = GeometryInfoFactory(m_workspace);
    TS_ASSERT_EQUALS(info.getL1(), 20.0);
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_API_GEOMETRYINFOFACTORY_TEST_H_ */
