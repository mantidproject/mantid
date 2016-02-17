#ifndef MANTID_API_GEOMETRYINFOTEST_H_
#define MANTID_API_GEOMETRYINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/GeometryInfo.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class GeometryInfoTest : public CxxTest::TestSuite {
public:
  static GeometryInfoTest *createSuite() { return new GeometryInfoTest(); }
  static void destroySuite(GeometryInfoTest *suite) { delete suite; }

  GeometryInfoTest()
      : workspace(WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            5, 1, true, true)),
        m_instrument_info(*workspace) {}

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(0))));
  }

  void test_isMonitor() {
    TS_ASSERT_EQUALS(GeometryInfo(m_instrument_info,
                                  *(workspace->getSpectrum(0))).isMonitor(),
                     false);
    TS_ASSERT_EQUALS(GeometryInfo(m_instrument_info,
                                  *(workspace->getSpectrum(1))).isMonitor(),
                     false);
    TS_ASSERT_EQUALS(GeometryInfo(m_instrument_info,
                                  *(workspace->getSpectrum(2))).isMonitor(),
                     false);
    TS_ASSERT_EQUALS(GeometryInfo(m_instrument_info,
                                  *(workspace->getSpectrum(3))).isMonitor(),
                     true);
    TS_ASSERT_EQUALS(GeometryInfo(m_instrument_info,
                                  *(workspace->getSpectrum(4))).isMonitor(),
                     true);
  }

  void test_isMasked() {
    auto ws = WorkspaceCreationHelper::maskSpectra(workspace, {0, 3});
    auto instrument_info = GeometryInfoFactory(*ws);
    TS_ASSERT_EQUALS(
        GeometryInfo(instrument_info, *(ws->getSpectrum(0))).isMasked(), true);
    TS_ASSERT_EQUALS(
        GeometryInfo(instrument_info, *(ws->getSpectrum(1))).isMasked(), false);
    TS_ASSERT_EQUALS(
        GeometryInfo(instrument_info, *(ws->getSpectrum(2))).isMasked(), false);
    TS_ASSERT_EQUALS(
        GeometryInfo(instrument_info, *(ws->getSpectrum(3))).isMasked(), true);
    TS_ASSERT_EQUALS(
        GeometryInfo(instrument_info, *(ws->getSpectrum(4))).isMasked(), false);
  }

  void test_getL1() {
    auto info = GeometryInfo(m_instrument_info, *(workspace->getSpectrum(0)));
    TS_ASSERT_EQUALS(info.getL1(), 20.0);
  }

  void test_getL2() {
    double x2 = 5.0 * 5.0;
    double y2 = 2.0 * 2.0 * 0.05 * 0.05;
    TS_ASSERT_EQUALS(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(0))).getL2(),
        sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(1))).getL2(),
        sqrt(x2 + 0 * 0 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(2))).getL2(),
        sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(3))).getL2(),
        -9.0);
    TS_ASSERT_EQUALS(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(4))).getL2(),
        -2.0);
  }

  void test_getTwoTheta() {
    TS_ASSERT_DELTA(GeometryInfo(m_instrument_info,
                                 *(workspace->getSpectrum(0))).getTwoTheta(),
                    0.0199973, 1e-6);
    TS_ASSERT_DELTA(GeometryInfo(m_instrument_info,
                                 *(workspace->getSpectrum(1))).getTwoTheta(),
                    0.0, 1e-6);
    TS_ASSERT_DELTA(GeometryInfo(m_instrument_info,
                                 *(workspace->getSpectrum(2))).getTwoTheta(),
                    0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_getTwoThetaLegacy() {
    auto info = GeometryInfo(m_instrument_info, *(workspace->getSpectrum(2)));
    TS_ASSERT_EQUALS(info.getTwoTheta(),
                     workspace->detectorTwoTheta(info.getDetector()));
  }

  void test_getSignedTwoTheta() {
    TS_ASSERT_DELTA(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(0)))
            .getSignedTwoTheta(),
        -0.0199973, 1e-6);
    TS_ASSERT_DELTA(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(1)))
            .getSignedTwoTheta(),
        0.0, 1e-6);
    TS_ASSERT_DELTA(
        GeometryInfo(m_instrument_info, *(workspace->getSpectrum(2)))
            .getSignedTwoTheta(),
        0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorSignedTwoTheta(), which might
  // be removed at some point.
  void test_getSignedTwoThetaLegacy() {
    auto info = GeometryInfo(m_instrument_info, *(workspace->getSpectrum(2)));
    TS_ASSERT_EQUALS(info.getSignedTwoTheta(),
                     workspace->detectorSignedTwoTheta(info.getDetector()));
  }

private:
  Workspace2D_sptr workspace;
  GeometryInfoFactory m_instrument_info;
};

#endif /* MANTID_API_GEOMETRYINFOTEST_H_ */
