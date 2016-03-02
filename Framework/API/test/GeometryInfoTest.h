#ifndef MANTID_API_GEOMETRYINFOTEST_H_
#define MANTID_API_GEOMETRYINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/make_unique.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidAPI/GeometryInfo.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidKernel/MultiThreaded.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;

class GeometryInfoTest : public CxxTest::TestSuite {
public:
  static GeometryInfoTest *createSuite() { return new GeometryInfoTest(); }
  static void destroySuite(GeometryInfoTest *suite) { delete suite; }

  GeometryInfoTest() : m_workspace(nullptr) {
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);

    bool includeMonitors = true;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);

    std::set<int64_t> toMask{0, 3};
    ParameterMap &pmap = m_workspace.instrumentParameters();
    for (size_t i = 0; i < m_workspace.getNumberHistograms(); ++i) {
      if (toMask.find(i) != toMask.end()) {
        IDetector_const_sptr det = m_workspace.getDetector(i);
        pmap.addBool(det.get(), "masked", true);
      }
    }

    m_factory = Kernel::make_unique<GeometryInfoFactory>(m_workspace);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0))));
  }

  void test_isMonitor() {
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0))).isMonitor(),
        false);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(1))).isMonitor(),
        false);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2))).isMonitor(),
        false);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(3))).isMonitor(),
        true);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(4))).isMonitor(),
        true);
  }

  void test_isMasked() {
    auto factory = GeometryInfoFactory(m_workspace);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0))).isMasked(),
        true);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(1))).isMasked(),
        false);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2))).isMasked(),
        false);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(3))).isMasked(),
        true);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(4))).isMasked(),
        false);
  }

  void test_getL1() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(0)));
    TS_ASSERT_EQUALS(info.getL1(), 20.0);
  }

  void test_getL2() {
    double x2 = 5.0 * 5.0;
    double y2 = 2.0 * 2.0 * 0.05 * 0.05;
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0))).getL2(),
        sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(1))).getL2(),
        sqrt(x2 + 0 * 0 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2))).getL2(),
        sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(3))).getL2(), -9.0);
    TS_ASSERT_EQUALS(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(4))).getL2(), -2.0);
  }

  void test_getTwoTheta() {
    TS_ASSERT_DELTA(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0))).getTwoTheta(),
        0.0199973, 1e-6);
    TS_ASSERT_DELTA(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(1))).getTwoTheta(),
        0.0, 1e-6);
    TS_ASSERT_DELTA(
        GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2))).getTwoTheta(),
        0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_getTwoThetaLegacy() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(2)));
    TS_ASSERT_EQUALS(info.getTwoTheta(),
                     m_workspace.detectorTwoTheta(info.getDetector()));
  }

  void test_getSignedTwoTheta() {
    TS_ASSERT_DELTA(GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0)))
                        .getSignedTwoTheta(),
                    -0.0199973, 1e-6);
    TS_ASSERT_DELTA(GeometryInfo(*m_factory, *(m_workspace.getSpectrum(1)))
                        .getSignedTwoTheta(),
                    0.0, 1e-6);
    TS_ASSERT_DELTA(GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2)))
                        .getSignedTwoTheta(),
                    0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorSignedTwoTheta(), which might
  // be removed at some point.
  void test_getSignedTwoThetaLegacy() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(2)));
    TS_ASSERT_EQUALS(info.getSignedTwoTheta(),
                     m_workspace.detectorSignedTwoTheta(info.getDetector()));
  }

  void test_multithreaded_access_l2() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(2)));
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 100; ++i) {
      info.getL2();
    }
  }

  void test_multithreaded_access_two_theta() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(2)));
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 100; ++i) {
      info.getTwoTheta();
    }
  }

  void test_multithreaded_access_signed_two_theta() {
    GeometryInfo info(*m_factory, *(m_workspace.getSpectrum(2)));
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 100; ++i) {
      info.getSignedTwoTheta();
    }
  }

private:
  WorkspaceTester m_workspace;
  std::unique_ptr<GeometryInfoFactory> m_factory;
};

class GeometryInfoTestPerformance : public CxxTest::TestSuite {
public:
  static GeometryInfoTestPerformance *createSuite() {
    return new GeometryInfoTestPerformance();
  }
  static void destroySuite(GeometryInfoTestPerformance *suite) { delete suite; }

  GeometryInfoTestPerformance() {
    const size_t numberOfHistograms = 100000;
    const size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);
  }

  void test_single_access_multiple_spectrum() {
    /*
     * We are testing the effect of single access to multiple detector detector
     * l1 l2,
     * twoTheta information.
     */
    double result = 0.0;
    GeometryInfoFactory factory(m_workspace);

    for (size_t i = 0; i < m_workspace.getNumberHistograms(); ++i) {
      GeometryInfo geometryInfo(factory.create(i));
      result += geometryInfo.getL1();
      result += geometryInfo.getL2();
      result += geometryInfo.getTwoTheta();
    }
    // We are computing an using the result to fool the optimizer.
    TS_ASSERT(result > 0);
  }

  void test_typical_access_multiple_spectrum() {
    /*
     * We are testing the effect of typical access to multiple detector l1 l2,
     * twoTheta information.
     */
    double result = 0.0;
    GeometryInfoFactory factory(m_workspace);

    for (size_t i = 0; i < m_workspace.getNumberHistograms(); ++i) {
      for (size_t j = 0; j < 10; ++j) {
        auto geometryInfo = factory.create(i);
        result += geometryInfo.getL1();
        result += geometryInfo.getL2();
        result += geometryInfo.getTwoTheta();
      }
    }
    // We are computing an using the result to fool the optimizer.
    TS_ASSERT(result > 0);
  }

  void test_multiple_access_single_spectrum() {
    /*
     * We are testing the effect of repeated (probably unrealistic) access to
     * the same detector l1 l2,
     * twoTheta information.
     */
    double result = 0.0;
    GeometryInfoFactory factory(m_workspace);
    auto geometryInfo = factory.create(0);
    for (size_t i = 0; i < 100000; ++i) {

      result += geometryInfo.getL1();
      result += geometryInfo.getL2();
      result += geometryInfo.getTwoTheta();
    }
    // We are computing an using the result to fool the optimizer.
    TS_ASSERT(result > 0);
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_API_GEOMETRYINFOTEST_H_ */
