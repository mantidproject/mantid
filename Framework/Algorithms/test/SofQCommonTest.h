// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SOFQCOMMONTEST_H
#define MANTID_ALGORITHMS_SOFQCOMMONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SofQCommon.h"

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class SofQCommonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SofQCommonTest *createSuite() { return new SofQCommonTest(); }
  static void destroySuite(SofQCommonTest *suite) { delete suite; }

  void test_initDirectGeometryEiFromSampleLogs() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Direct");
    Algorithms::SofQCommon s;
    auto ws = create2DWorkspaceWithFullInstrument(1, 1);
    const double Ei{2.3};
    ws->mutableRun().addProperty("Ei", Ei);
    s.initCachedValues(*ws, &alg);
    TS_ASSERT_EQUALS(s.m_emode, 1)
    TS_ASSERT_EQUALS(s.m_efixed, Ei)
  }

  void test_initDirectGeometryEiFromAlgorithm() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Direct");
    const double Ei{2.3};
    alg.setProperty("EFixed", Ei);
    Algorithms::SofQCommon s;
    auto ws = create2DWorkspaceWithFullInstrument(1, 1);
    s.initCachedValues(*ws, &alg);
    TS_ASSERT_EQUALS(s.m_emode, 1)
    TS_ASSERT_EQUALS(s.m_efixed, Ei)
  }

  void test_initIndirectGeometry() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    const double Ef{2.3};
    alg.setProperty("EFixed", Ef);
    Algorithms::SofQCommon s;
    auto ws = create2DWorkspaceWithFullInstrument(1, 1);
    s.initCachedValues(*ws, &alg);
    TS_ASSERT_EQUALS(s.m_emode, 2)
    TS_ASSERT_EQUALS(s.m_efixed, Ef)
  }

  void test_getEFixedDirectGeometry() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Direct");
    const double Ei{2.3};
    alg.setProperty("EFixed", Ei);
    Algorithms::SofQCommon s;
    auto ws = create2DWorkspaceWithFullInstrument(13, 1);
    s.initCachedValues(*ws, &alg);
    const auto &detectorInfo = ws->detectorInfo();
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      TS_ASSERT_EQUALS(s.getEFixed(detectorInfo.detector(i)), Ei);
    }
  }

  void test_getEFixedFromDetectorsIndirectGeometry() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    Algorithms::SofQCommon s;
    const size_t nHist{13};
    auto ws = create2DWorkspaceWithFullInstrument(nHist, 1);
    for (size_t i = 0; i < nHist; ++i) {
      const auto comp = "pixel-" + std::to_string(i) + ")";
      const auto Ef = static_cast<double>(i) + 0.38;
      setEFixed(ws, comp, Ef);
    }
    s.initCachedValues(*ws, &alg);
    const auto &detectorInfo = ws->detectorInfo();
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      const auto E = static_cast<double>(i) + 0.38;
      TS_ASSERT_EQUALS(s.getEFixed(detectorInfo.detector(i)), E);
    }
  }

  void test_getEFixedIndirectGeometryAlgorithPropertiesOverrideIPF() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    const double Ef{2.3};
    alg.setProperty("EFixed", Ef);
    Algorithms::SofQCommon s;
    const size_t nHist{13};
    auto ws = create2DWorkspaceWithFullInstrument(nHist, 1);
    for (size_t i = 0; i < nHist; ++i) {
      const auto comp = "pixel-" + std::to_string(i) + ")";
      const auto eParam = static_cast<double>(i) + 0.77;
      setEFixed(ws, comp, eParam);
    }
    s.initCachedValues(*ws, &alg);
    const auto &detectorInfo = ws->detectorInfo();
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      TS_ASSERT_EQUALS(s.getEFixed(detectorInfo.detector(i)), Ef);
    }
  }

  void test_qBinHintsDirect() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Direct");
    Algorithms::SofQCommon s;
    const size_t nBins{23};
    auto ws = create2DWorkspaceWithFullInstrument(1, nBins);
    const auto minDeltaE = ws->x(0).front();
    const auto maxDeltaE = ws->x(0).back();
    const auto Ei = 2. * static_cast<double>(nBins);
    ws->mutableRun().addProperty("Ei", Ei);
    const auto minQ = directQ(Ei, minDeltaE);
    const auto maxQ = directQ(Ei, maxDeltaE);
    s.initCachedValues(*ws, &alg);
    double minE;
    double maxE;
    ws->getXMinMax(minE, maxE);
    std::pair<double, double> minmaxQ;
    TS_ASSERT_THROWS_NOTHING(minmaxQ = s.qBinHints(*ws, minE, maxE))
    TS_ASSERT(minmaxQ.first < minmaxQ.second)
    TS_ASSERT_EQUALS(minmaxQ.first, minQ)
    TS_ASSERT_DELTA(minmaxQ.second, maxQ, 1e-12)
  }

  void test_qBinHintsIndirect() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    Algorithms::SofQCommon s;
    const size_t nBins{23};
    const size_t nDets{2};
    auto ws = create2DWorkspaceWithFullInstrument(nDets, nBins);
    const auto &spectrumInfo = ws->spectrumInfo();
    const auto twoTheta0 = spectrumInfo.twoTheta(0);
    const auto twoTheta1 = spectrumInfo.twoTheta(1);
    const double eFixed0{3.7};
    const double eFixed1{2.3};
    setEFixed(ws, "pixel-0)", eFixed0);
    setEFixed(ws, "pixel-1)", eFixed1);
    const auto minDeltaE = ws->x(0).front();
    const auto maxDeltaE = ws->x(0).back();
    const auto minQ = std::min(indirectQ(eFixed0, minDeltaE, twoTheta0),
                               indirectQ(eFixed1, minDeltaE, twoTheta1));
    const auto maxQ = std::max(indirectQ(eFixed0, maxDeltaE, twoTheta1),
                               indirectQ(eFixed1, maxDeltaE, twoTheta1));
    s.initCachedValues(*ws, &alg);
    double minE;
    double maxE;
    ws->getXMinMax(minE, maxE);
    const auto minmaxQ = s.qBinHints(*ws, minE, maxE);
    TS_ASSERT_EQUALS(minmaxQ.first, minQ)
    TS_ASSERT_DELTA(minmaxQ.second, maxQ, 1e-12)
  }

  void testDirectQ() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Direct");
    Algorithms::SofQCommon s;
    const size_t nBins{1};
    auto ws = create2DWorkspaceWithFullInstrument(5, nBins);
    const auto Ei = 4.2;
    ws->mutableRun().addProperty("Ei", Ei);
    s.initCachedValues(*ws, &alg);
    const auto &detectorInfo = ws->detectorInfo();
    const auto testDeltaE = -Ei / 1.8;
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      const auto twoTheta = detectorInfo.twoTheta(i);
      const auto expected = directQ(Ei, testDeltaE, twoTheta);
      TS_ASSERT_EQUALS(s.q(testDeltaE, twoTheta, nullptr), expected)
    }
  }

  void testIndirectQ() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    Algorithms::SofQCommon s;
    const size_t nBins{1};
    auto ws = create2DWorkspaceWithFullInstrument(2, nBins);
    const std::array<double, 2> Ef{{3.7, 2.3}};
    setEFixed(ws, "pixel-0)", Ef[0]);
    setEFixed(ws, "pixel-1)", Ef[1]);
    s.initCachedValues(*ws, &alg);
    const auto &detectorInfo = ws->detectorInfo();
    const auto testDeltaE = -1.8;
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      const auto &det = detectorInfo.detector(i);
      const auto twoTheta = detectorInfo.twoTheta(i);
      const auto expected = indirectQ(Ef[i], testDeltaE, twoTheta);
      TS_ASSERT_DELTA(s.q(testDeltaE, twoTheta, &det), expected, 1e-12)
    }
  }

private:
  static double k(const double E) {
    using namespace Mantid;
    using PhysicalConstants::NeutronMass;
    using PhysicalConstants::h_bar;
    using PhysicalConstants::meV;
    return std::sqrt(2 * NeutronMass * E * meV) / h_bar * 1e-10;
  }

  static double indirectQ(const double Ef, const double DeltaE,
                          const double twoTheta = 0.) {
    const auto kf = k(Ef);
    const auto Ei = Ef + DeltaE;
    const auto ki = k(Ei);
    return std::sqrt(ki * ki + kf * kf - 2. * ki * kf * std::cos(twoTheta));
  }

  static double directQ(const double Ei, const double DeltaE,
                        const double twoTheta = 0.) {
    const auto ki = k(Ei);
    const auto Ef = Ei - DeltaE;
    const auto kf = k(Ef);
    return std::sqrt(ki * ki + kf * kf - 2. * ki * kf * std::cos(twoTheta));
  }

  static void setEFixed(Mantid::API::MatrixWorkspace_sptr ws,
                        const std::string &component, const double eFixed) {
    using namespace Mantid;
    auto alg = API::AlgorithmManager::Instance().createUnmanaged(
        "SetInstrumentParameter");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("Workspace", ws);
    alg->setProperty("ComponentName", component);
    alg->setProperty("ParameterName", "EFixed");
    alg->setProperty("ParameterType", "Number");
    alg->setProperty("Value", std::to_string(eFixed));
    alg->execute();
  }
};

#endif /* MANTID_ALGORITHMS_SOFQCOMMONTEST_H */
