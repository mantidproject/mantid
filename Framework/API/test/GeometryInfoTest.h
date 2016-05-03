#ifndef MANTID_API_GEOMETRYINFOTEST_H_
#define MANTID_API_GEOMETRYINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/make_unique.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidAPI/GeometryInfo.h"

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

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
    GeometryInfoFactory factory(m_workspace);
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
    auto info = GeometryInfo(*m_factory, *(m_workspace.getSpectrum(0)));
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
    auto info = GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2)));
    auto det = info.getDetector();
    TS_ASSERT_EQUALS(info.getTwoTheta(), m_workspace.detectorTwoTheta(*det));
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
    auto info = GeometryInfo(*m_factory, *(m_workspace.getSpectrum(2)));
    auto det = info.getDetector();
    TS_ASSERT_EQUALS(info.getSignedTwoTheta(),
                     m_workspace.detectorSignedTwoTheta(*det));
  }

private:
  WorkspaceTester m_workspace;
  std::unique_ptr<GeometryInfoFactory> m_factory;
};

#endif /* MANTID_API_GEOMETRYINFOTEST_H_ */
