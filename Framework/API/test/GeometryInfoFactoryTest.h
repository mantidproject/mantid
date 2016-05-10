#ifndef MANTID_API_GEOMETRYINFOFACTORY_TEST_H_
#define MANTID_API_GEOMETRYINFOFACTORY_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidAPI/GeometryInfoFactory.h"
#include "MantidAPI/GeometryInfo.h"

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

    m_workspaceNoInstrument.init(numberOfHistograms, numberOfBins,
                                 numberOfBins - 1);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(GeometryInfoFactory factory(m_workspace));
  }

  void test_create() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_THROWS_NOTHING(factory.create(0).getDetector());
  }

  void test_getInstrument() {
    GeometryInfoFactory factory(m_workspace);
    // This might fail if we get a null instrument, so we test for throw. Since
    // workspace->getInstrument() creates a copy of the instrument there is no
    // point in attempting to verify that the pointer is "correct".
    TS_ASSERT_THROWS_NOTHING(factory.getInstrument());
  }

  void test_getSource() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_THROWS_NOTHING(factory.getSource());
  }

  void test_getSample() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_THROWS_NOTHING(factory.getSample());
  }

  void test_getSourcePos() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_EQUALS(factory.getSourcePos(), Kernel::V3D(0.0, 0.0, -20.0));
  }

  void test_getSamplePos() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_EQUALS(factory.getSamplePos(), Kernel::V3D(0.0, 0.0, 0.0));
  }

  void test_getL1() {
    GeometryInfoFactory factory(m_workspace);
    TS_ASSERT_EQUALS(factory.getL1(), 20.0);
  }

  void test_getSource_no_instrument() {
    GeometryInfoFactory factory(m_workspaceNoInstrument);
    TS_ASSERT_THROWS(factory.getSource(), std::runtime_error);
  }

  void test_getSourcePos_no_instrument() {
    GeometryInfoFactory factory(m_workspaceNoInstrument);
    TS_ASSERT_THROWS(factory.getSourcePos(), std::runtime_error);
  }

  void test_getSample_no_instrument() {
    GeometryInfoFactory factory(m_workspaceNoInstrument);
    TS_ASSERT_THROWS(factory.getSample(), std::runtime_error);
  }

  void test_getSamplePos_no_instrument() {
    GeometryInfoFactory factory(m_workspaceNoInstrument);
    TS_ASSERT_THROWS(factory.getSamplePos(), std::runtime_error);
  }

  void test_getL1_no_instrument() {
    GeometryInfoFactory factory(m_workspaceNoInstrument);
    TS_ASSERT_THROWS(factory.getL1(), std::runtime_error);
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceNoInstrument;
};

class GeometryInfoFactoryTestPerformance : public CxxTest::TestSuite {
public:
  static GeometryInfoFactoryTestPerformance *createSuite() {
    return new GeometryInfoFactoryTestPerformance();
  }
  static void destroySuite(GeometryInfoFactoryTestPerformance *suite) {
    delete suite;
  }

  GeometryInfoFactoryTestPerformance() : m_workspace(nullptr) {
    size_t numberOfHistograms = 10000;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);
  }

  void test_typical() {
    // Typically:
    // - workspace with > 10k histograms
    // - need L1, L2, and 2-theta
    // Note that the instrument in this case is extremely simple, with few
    // detectors and no parameters, so the actual performance will be worse.
    double result = 0.0;
    GeometryInfoFactory factory(m_workspace);
    for (size_t i = 0; i < 10000; ++i) {
      auto info = factory.create(i);
      result += info.getL1();
      result += info.getL2();
      result += info.getTwoTheta();
    }
    // We are computing an using the result to fool the optimizer.
    TS_ASSERT_DELTA(result, 5214709.740869, 1e-6);
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_API_GEOMETRYINFOFACTORY_TEST_H_ */
