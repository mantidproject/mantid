// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYSUMINQTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYSUMINQTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometrySumInQ.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ReflectometrySumInQ;
using Mantid::Geometry::deg2rad;

class ReflectometrySumInQTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometrySumInQTest *createSuite() {
    return new ReflectometrySumInQTest();
  }
  static void destroySuite(ReflectometrySumInQTest *suite) { delete suite; }

  static Mantid::API::MatrixWorkspace_sptr
  convertToWavelength(Mantid::API::MatrixWorkspace_sptr ws) {
    using namespace Mantid;
    auto toWavelength =
        API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    toWavelength->initialize();
    toWavelength->setChild(true);
    toWavelength->setProperty("InputWorkspace", ws);
    toWavelength->setProperty("OutputWorkspace", "_unused_for_child");
    toWavelength->setProperty("Target", "Wavelength");
    toWavelength->setProperty("EMode", "Elastic");
    toWavelength->execute();
    return toWavelength->getProperty("OutputWorkspace");
  }

  static Mantid::API::MatrixWorkspace_sptr
  detectorsOnly(Mantid::API::MatrixWorkspace_sptr ws) {
    using namespace Mantid;
    auto &specturmInfo = ws->spectrumInfo();
    std::vector<size_t> detectorIndices;
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      if (specturmInfo.isMonitor(i)) {
        continue;
      }
      detectorIndices.emplace_back(i);
    }
    auto extractDetectors =
        API::AlgorithmManager::Instance().createUnmanaged("ExtractSpectra");
    extractDetectors->initialize();
    extractDetectors->setChild(true);
    extractDetectors->setProperty("InputWorkspace", ws);
    extractDetectors->setProperty("OutputWorkspace", "_unused_for_child");
    extractDetectors->setProperty("WorkspaceIndexList", detectorIndices);
    extractDetectors->execute();
    return extractDetectors->getProperty("OutputWorkspace");
  }

  void test_init() {
    ReflectometrySumInQ alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_sumSingleHistogram() {
    using namespace Mantid;
    auto inputWS = testWorkspace();
    inputWS = detectorsOnly(inputWS);
    inputWS = convertToWavelength(inputWS);
    const auto &Ys = inputWS->y(0);
    const auto totalY = std::accumulate(Ys.cbegin(), Ys.cend(), 0.);
    const std::array<bool, 2> flatSampleOptions{{true, false}};
    for (const auto isFlatSample : flatSampleOptions) {
      for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
        ReflectometrySumInQ alg;
        alg.setChild(true);
        alg.setRethrows(true);
        TS_ASSERT_THROWS_NOTHING(alg.initialize())
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("InputWorkspaceIndexSet", std::to_string(i)))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
        TS_ASSERT_THROWS_NOTHING(
            alg.setProperty("BeamCentre", static_cast<double>(i)))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", isFlatSample))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludePartialBins", true))
        TS_ASSERT_THROWS_NOTHING(alg.execute())
        API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
        TS_ASSERT(outputWS);
        TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
        auto &inXs = inputWS->x(i);
        const auto binWidth =
            (inXs.back() - inXs.front()) / static_cast<double>(inXs.size() - 1);
        auto &outXs = outputWS->x(0);
        for (size_t binIndex = 0; binIndex < outXs.size() - 1; ++binIndex) {
          TS_ASSERT_DELTA(outXs[binIndex + 1] - outXs[binIndex], binWidth,
                          1e-12)
        }
        auto &Ys = outputWS->y(0);
        const auto totalYSummedInQ =
            std::accumulate(Ys.cbegin(), Ys.cend(), 0.0);
        TS_ASSERT_DELTA(totalYSummedInQ, totalY, 1e-10)
      }
    }
  }

  void test_sumEntireWorkspace() {
    using namespace Mantid;
    auto inputWS = testWorkspace();
    inputWS = detectorsOnly(inputWS);
    inputWS = convertToWavelength(inputWS);
    auto &Ys = inputWS->y(0);
    double totalY{0.0};
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      totalY += std::accumulate(Ys.cbegin(), Ys.cend(), 0.0);
    }
    const std::array<bool, 2> flatSampleOptions{{true, false}};
    for (const auto isFlatSample : flatSampleOptions) {
      // Loop over possible beam centres.
      for (size_t beamCentre = 0; beamCentre < inputWS->getNumberHistograms();
           ++beamCentre) {
        ReflectometrySumInQ alg;
        alg.setChild(true);
        alg.setRethrows(true);
        TS_ASSERT_THROWS_NOTHING(alg.initialize())
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("InputWorkspaceIndexSet", "0, 1, 2"))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
        TS_ASSERT_THROWS_NOTHING(
            alg.setProperty("BeamCentre", static_cast<double>(beamCentre)))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", isFlatSample))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludePartialBins", true))
        TS_ASSERT_THROWS_NOTHING(alg.execute())
        API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
        TS_ASSERT(outputWS);
        TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
        auto &inXs = inputWS->x(beamCentre);
        const auto binWidth =
            (inXs.back() - inXs.front()) / static_cast<double>(inXs.size() - 1);
        auto &outXs = outputWS->x(0);
        for (size_t binIndex = 0; binIndex < outXs.size() - 1; ++binIndex) {
          TS_ASSERT_DELTA(outXs[binIndex + 1] - outXs[binIndex], binWidth,
                          1e-12)
        }
        auto &Ys = outputWS->y(0);
        const auto totalYSummedInQ =
            std::accumulate(Ys.cbegin(), Ys.cend(), 0.0);
        TS_ASSERT_DELTA(totalYSummedInQ, totalY, 1e-10)
      }
    }
  }

  void test_excludePartialBins() {
    using namespace Mantid;
    auto inputWS = testWorkspace();
    inputWS = detectorsOnly(inputWS);
    inputWS = convertToWavelength(inputWS);
    const std::array<bool, 2> flatSampleOptions{{true, false}};
    for (const auto isFlatSample : flatSampleOptions) {
      for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
        ReflectometrySumInQ alg;
        alg.setChild(true);
        alg.setRethrows(true);
        TS_ASSERT_THROWS_NOTHING(alg.initialize())
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("InputWorkspaceIndexSet", std::to_string(i)))
        TS_ASSERT_THROWS_NOTHING(
            alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
        TS_ASSERT_THROWS_NOTHING(
            alg.setProperty("BeamCentre", static_cast<double>(i)))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", isFlatSample))
        TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludePartialBins", false))
        TS_ASSERT_THROWS_NOTHING(alg.execute())
        API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
        TS_ASSERT(outputWS);
        TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
        auto hist = outputWS->histogram(0);
        const auto firstItem = *hist.begin();
        for (const auto &i : hist) {
          TS_ASSERT_DELTA(i.binWidth(), firstItem.binWidth(), 1e-12)
          TS_ASSERT_DELTA(i.counts(), firstItem.counts(), 1e-1)
          TS_ASSERT_DELTA(i.countStandardDeviation(),
                          firstItem.countStandardDeviation(), 1e-1)
        }
      }
    }
  }

  void test_isTwoThetaSignAgnostic() {
    using namespace Mantid;
    auto inputWS = testWorkspace(0, 51); // One spectrum is monitor
    inputWS = detectorsOnly(inputWS);
    inputWS = convertToWavelength(inputWS);
    const auto &spectrumInfo = inputWS->spectrumInfo();
    constexpr int spectrum1{1};
    const int spectrum2{static_cast<int>(spectrumInfo.size()) - 2};
    TS_ASSERT_LESS_THAN(spectrumInfo.signedTwoTheta(spectrum1), 0.)
    TS_ASSERT_LESS_THAN(0., spectrumInfo.signedTwoTheta(spectrum2))
    double summedInLambda{0.};
    for (auto i : std::array<int, 2>{{spectrum1, spectrum2}}) {
      const auto &Ys = inputWS->y(i);
      summedInLambda += std::accumulate(Ys.cbegin(), Ys.cend(), 0.0);
    }
    std::ostringstream indexSetValue;
    indexSetValue << spectrum1 << "," << spectrum2;
    const std::array<bool, 2> flatSampleOptions{{true, false}};
    for (const auto isFlatSample : flatSampleOptions) {
      ReflectometrySumInQ alg;
      alg.setChild(true);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("InputWorkspaceIndexSet", indexSetValue.str()))
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("BeamCentre", static_cast<double>(spectrum1)))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", isFlatSample))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludePartialBins", true))
      TS_ASSERT_THROWS_NOTHING(alg.execute())
      API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT(outputWS);
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
      auto &Ys = outputWS->y(0);
      const auto summedInQ = std::accumulate(Ys.cbegin(), Ys.cend(), 0.0);
      TS_ASSERT_DELTA(summedInQ, summedInLambda, 1e-10)
    }
  }

  void test_monitorNextToDetectorsThrows() {
    auto inputWS = testWorkspace();
    inputWS = convertToWavelength(inputWS);
    ReflectometrySumInQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    constexpr size_t monitorIdx{0};
    constexpr size_t detectorIdx{1};
    TS_ASSERT(inputWS->spectrumInfo().isMonitor(monitorIdx))
    TS_ASSERT(!inputWS->spectrumInfo().isMonitor(detectorIdx))
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaceIndexSet",
                                                  std::to_string(detectorIdx)))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BeamCentre", static_cast<double>(detectorIdx)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", true))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            e.what(),
                            std::string("Some invalid Properties found"))
  }

  void test_monitorInIndexSetThrows() {
    auto inputWS = testWorkspace();
    inputWS = convertToWavelength(inputWS);
    ReflectometrySumInQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    const size_t monitorIdx{0};
    TS_ASSERT(inputWS->spectrumInfo().isMonitor(monitorIdx))
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaceIndexSet",
                                                  std::to_string(monitorIdx)))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BeamCentre", static_cast<double>(monitorIdx)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", true))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            e.what(),
                            std::string("Some invalid Properties found"))
  }

  void test_BeamCentreNotInIndexSetThrows() {
    auto inputWS = testWorkspace();
    inputWS = convertToWavelength(inputWS);
    inputWS = detectorsOnly(inputWS);
    ReflectometrySumInQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaceIndexSet", "0, 1"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BeamCentre", 2.))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlatSample", true))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            e.what(),
                            std::string("Some invalid Properties found"))
  }

private:
  static Mantid::API::MatrixWorkspace_sptr
  testWorkspace(const double centreTwoThetaDegrees = 0.87,
                const int nSpectra = 4) {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    constexpr double startX{0.1};
    const Kernel::V3D slit1Pos{0., 0., -2.};
    const Kernel::V3D slit2Pos{0., 0., -1.};
    constexpr double vg1{0.5};
    constexpr double vg2{1.};
    const Kernel::V3D sourcePos{0., 0., -50.};
    const Kernel::V3D monitorPos{0., 0., -0.5};
    const Kernel::V3D samplePos{
        0.,
        0.,
        0.,
    };
    const double twoTheta{centreTwoThetaDegrees * deg2rad};
    constexpr double detectorHeight{0.001};
    constexpr double l2{2.3};
    const auto y = l2 * std::sin(twoTheta);
    const auto z = l2 * std::cos(twoTheta);
    const Kernel::V3D centrePos{0., y, z};
    constexpr int nBins{50};
    return create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        startX, detectorHeight, slit1Pos, slit2Pos, vg1, vg2, sourcePos,
        monitorPos, samplePos, centrePos, nSpectra, nBins);
  }
};

class ReflectometrySumInQTestPerformance : public CxxTest::TestSuite {
public:
  static ReflectometrySumInQTestPerformance *createSuite() {
    return new ReflectometrySumInQTestPerformance();
  }
  static void destroySuite(ReflectometrySumInQTestPerformance *suite) {
    delete suite;
  }

  ReflectometrySumInQTestPerformance() {
    using namespace Mantid;
    using namespace WorkspaceCreationHelper;
    constexpr double startX{0.};
    const Kernel::V3D slit1Pos{0., 0., -2.};
    const Kernel::V3D slit2Pos{0., 0., -1.};
    constexpr double vg1{0.5};
    constexpr double vg2{1.};
    const Kernel::V3D sourcePos{0., 0., -50.};
    const Kernel::V3D monitorPos{0., 0., -0.5};
    const Kernel::V3D samplePos{
        0.,
        0.,
        0.,
    };
    constexpr double twoTheta{5.87 * deg2rad};
    constexpr double detectorHeight{0.001};
    constexpr double l2{2.3};
    const auto y = l2 * std::sin(twoTheta);
    const auto z = l2 * std::cos(twoTheta);
    const Kernel::V3D centrePos{0., y, z};
    constexpr int nSpectra{101}; // One spectrum is monitor
    constexpr int nBins{200};
    constexpr double binWidth{1250.};
    m_workspace = create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        startX, detectorHeight, slit1Pos, slit2Pos, vg1, vg2, sourcePos,
        monitorPos, samplePos, centrePos, nSpectra, nBins, binWidth);
    m_workspace = ReflectometrySumInQTest::convertToWavelength(m_workspace);
    m_workspace = ReflectometrySumInQTest::detectorsOnly(m_workspace);
    m_fullIndexSet.assign(m_workspace->getNumberHistograms(), 0);
    std::iota(m_fullIndexSet.begin(), m_fullIndexSet.end(), 0);
  }

  void test_typical() {
    ReflectometrySumInQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_workspace);
    alg.setProperty("InputWorkspaceIndexSet", m_fullIndexSet);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg.setProperty("BeamCentre", 49.);
    alg.setProperty("FlatSample", true);
    for (int repetitions = 0; repetitions < 1000; ++repetitions) {
      alg.execute();
    }
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  std::vector<int64_t> m_fullIndexSet;
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYSUMINQTEST_H_ */
