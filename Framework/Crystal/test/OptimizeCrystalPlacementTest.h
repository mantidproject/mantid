// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef OPTIMIZECRYSTALPLACEMENTTEST_H_
#define OPTIMIZECRYSTALPLACEMENTTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/OptimizeCrystalPlacement.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::Crystal::OptimizeCrystalPlacement;
using Mantid::Crystal::PeakHKLErrors;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::Goniometer;
using Mantid::Geometry::Instrument;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

namespace {
using StringPairVector = std::vector<std::pair<std::string, std::string>>;
using ResultType = std::pair<PeaksWorkspace_sptr, ITableWorkspace_sptr>;

/**
 * Run the OptimizeCrystalPlacement algorithm
 * @param peaksWS Passed to the "PeaksWorkspace" property
 * @param args Additional vector of arguments.
 */
ResultType
runOptimizePlacement(const PeaksWorkspace_sptr &peaksWS,
                     const StringPairVector &args = StringPairVector()) {
  // This algorithm can not currently run without the ADS so add them and remove
  // after
  const std::string inputName("__runOptimizePlacement_PeaksWorkspace"),
      modPeaksName("__runOptimizePlacement_ModPeaksWorkspace"),
      fitTableName("__runOptimizePlacement_FitInfoTable");
  auto &ads = AnalysisDataService::Instance();
  ads.add(inputName, peaksWS);

  OptimizeCrystalPlacement alg;
  alg.initialize();
  alg.setPropertyValue("PeaksWorkspace", inputName);
  alg.setPropertyValue("ModifiedPeaksWorkspace", modPeaksName);
  alg.setPropertyValue("FitInfoTable", fitTableName);
  for (const auto &arg : args) {
    alg.setPropertyValue(arg.first, arg.second);
  }
  alg.execute();
  auto modifiedPeaksWS = ads.retrieveWS<PeaksWorkspace>(modPeaksName);
  auto fitInfoWS = ads.retrieveWS<ITableWorkspace>(fitTableName);
  ads.remove(inputName);
  ads.remove(modPeaksName);
  ads.remove(fitTableName);
  return std::make_pair(modifiedPeaksWS, fitInfoWS);
}
} // namespace

class OptimizeCrystalPlacementTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OptimizeCrystalPlacementTest *createSuite() {
    return new OptimizeCrystalPlacementTest();
  }
  static void destroySuite(OptimizeCrystalPlacementTest *suite) {
    delete suite;
  }

  void test_basic() {
    auto modPeaksNoFix = calculateBasicPlacement();
    Matrix<double> goniomMat, origGon5638Mat;
    const auto rotx = PeakHKLErrors::RotationMatrixAboutRegAxis(0.5, 'x');
    const auto roty = PeakHKLErrors::RotationMatrixAboutRegAxis(-1, 'y');

    const auto npeaks = modPeaksNoFix->getNumberPeaks();
    for (int i = 0; i < npeaks; ++i) {
      if (modPeaksNoFix->getPeak(i).getRunNumber() == 5638) {
        auto &peak = modPeaksNoFix->getPeak(i);
        if (origGon5638Mat.numRows() == 0) {
          origGon5638Mat = peak.getGoniometerMatrix();
          goniomMat = rotx * roty * origGon5638Mat;
        }
        peak.setGoniometerMatrix(goniomMat);
      }
    }
    Goniometer instGoniom(goniomMat);
    auto resultsFix5637 = runOptimizePlacement(
        modPeaksNoFix, {{"KeepGoniometerFixedfor", "5637"}});
    auto fitInfoFix5637 = resultsFix5637.second;
    const auto angles5638 = Goniometer(origGon5638Mat).getEulerAngles("YZY");

    for (size_t i = 0; i < fitInfoFix5637->rowCount(); ++i) {
      const std::string nm = fitInfoFix5637->String(i, 0);
      double d = 0.0;
      if (nm == "chi5638")
        d = angles5638[1] - fitInfoFix5637->Double(i, 1);
      else if (nm == "phi5638")
        d = angles5638[2] - fitInfoFix5637->Double(i, 1);
      else if (nm == "omega5638")
        d = angles5638[0] - fitInfoFix5637->Double(i, 1);
      TS_ASSERT_DELTA(d, 0, .4);
    }
  }

  void test_tilt() {
    auto modPeaksNoFix = calculateBasicPlacement();
    const auto rotx = PeakHKLErrors::RotationMatrixAboutRegAxis(0.5, 'x');
    const auto roty = PeakHKLErrors::RotationMatrixAboutRegAxis(-1, 'y');

    Matrix<double> goniomMat, origGon5638Mat;
    const auto npeaks = modPeaksNoFix->getNumberPeaks();
    for (int i = 0; i < npeaks; ++i) {
      if (modPeaksNoFix->getPeak(i).getRunNumber() == 5638) {
        auto &peak = modPeaksNoFix->getPeak(i);
        if (origGon5638Mat.numRows() == 0) {
          origGon5638Mat = peak.getGoniometerMatrix();
          goniomMat = rotx * roty * origGon5638Mat;
        }
        peak.setGoniometerMatrix(goniomMat);
      }
    }

    const auto tilt = PeakHKLErrors::RotationMatrixAboutRegAxis(1, 'x') *
                      PeakHKLErrors::RotationMatrixAboutRegAxis(-2, 'y') *
                      PeakHKLErrors::RotationMatrixAboutRegAxis(1.3, 'z');

    Matrix<double> origGon5637Mat;
    for (int i = 0; i < npeaks; ++i) {
      auto &peak = modPeaksNoFix->getPeak(i);
      const int runNum = peak.getRunNumber();

      if (runNum == 5637) {
        if (origGon5637Mat.numRows() == 0) {
          origGon5637Mat = peak.getGoniometerMatrix();
        }
        peak.setGoniometerMatrix(tilt * origGon5637Mat);
      } else
        peak.setGoniometerMatrix(tilt * origGon5638Mat);
    }

    const auto angles5638 = Goniometer(origGon5638Mat).getEulerAngles("YZY");
    const auto angles5637 = Goniometer(origGon5637Mat).getEulerAngles("YZY");
    const auto resultsTiltFixBoth = runOptimizePlacement(
        modPeaksNoFix, {{"KeepGoniometerFixedfor", "5637, 5638"},
                        {"OptimizeGoniometerTilt", "1"}});

    const auto table = resultsTiltFixBoth.second;
    V3D rotXYZ;
    for (size_t i = 0; i < table->rowCount(); ++i) {
      const std::string nm = table->String(i, 0);
      if (nm == "GonRotx")
        rotXYZ[0] = table->Double(i, 1);
      else if (nm == "GonRoty")
        rotXYZ[1] = table->Double(i, 1);
      else if (nm == "GonRotz")
        rotXYZ[2] = table->Double(i, 1);
    }

    const auto tilt2 =
        PeakHKLErrors::RotationMatrixAboutRegAxis(rotXYZ[0], 'x') *
        PeakHKLErrors::RotationMatrixAboutRegAxis(rotXYZ[1], 'y') *
        PeakHKLErrors::RotationMatrixAboutRegAxis(rotXYZ[2], 'z');
    const auto tiltChange = tilt2 * tilt;
    const auto angles5637a =
        Goniometer(tiltChange * origGon5637Mat).getEulerAngles("YZY");
    const auto angles5638a =
        Goniometer(tiltChange * origGon5638Mat).getEulerAngles("YZY");

    for (int i = 0; i < 3; i++) {
      TS_ASSERT_DELTA(angles5637[i], angles5637a[i], .3);
      TS_ASSERT_DELTA(angles5638[i], angles5638a[i], .3);
    }
  }

  void test_SamplePosition() {
    auto modPeaksNoFix = calculateBasicPlacement();
    auto peak = modPeaksNoFix->getPeak(0);
    auto inst = peak.getInstrument();
    const V3D sampPos(.0003, -.00025, .00015);
    peak.setSamplePos(sampPos);

    auto pmap = inst->getParameterMap();
    auto sample = inst->getSample();
    pmap->addPositionCoordinate(sample.get(), "x", sampPos.X());
    pmap->addPositionCoordinate(sample.get(), "y", sampPos.Y());
    pmap->addPositionCoordinate(sample.get(), "z", sampPos.Z());
    auto newInst =
        boost::make_shared<const Instrument>(inst->baseInstrument(), pmap);

    for (int i = 0; i < modPeaksNoFix->getNumberPeaks(); ++i) {
      modPeaksNoFix->getPeak(i).setInstrument(newInst);
    }

    // optimize
    const auto resultsSamplePos = runOptimizePlacement(
        modPeaksNoFix, {{"KeepGoniometerFixedfor", "5637, 5638"},
                        {"AdjustSampleOffsets", "1"}});
    const auto table = resultsSamplePos.second;
    TS_ASSERT_DELTA(table->Double(0, 1), -0.0003377231, 1.e-4);
    TS_ASSERT_DELTA(table->Double(1, 1), 0.0000897573, 1.e-4);
    TS_ASSERT_DELTA(table->Double(2, 1), -0.0002679569, 1.e-4);
  }

private:
  // Helper method to load peaks and optimize them in the basic
  // case. Caches the result the first time it is called
  PeaksWorkspace_sptr calculateBasicPlacement() {
    if (!m_inputPeaksWS) {
      m_inputPeaksWS = loadTestPeaksInput();
    }
    return runOptimizePlacement(m_inputPeaksWS).first;
  }

  PeaksWorkspace_sptr loadTestPeaksInput() {
    using Mantid::Crystal::LoadIsawPeaks;
    using Mantid::Crystal::LoadIsawUB;
    // Load peaks input file and UB
    LoadIsawPeaks alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("Filename", "TOPAZ_5637_8.peaks");
    alg.setProperty("OutputWorkspace", "__");
    alg.execute();
    Workspace_sptr peaksWS = alg.getProperty("OutputWorkspace");
    LoadIsawUB loadUB;
    loadUB.setChild(true);
    loadUB.initialize();
    loadUB.setProperty("InputWorkspace", peaksWS);
    loadUB.setProperty("Filename", "ls5637.mat");
    loadUB.execute();
    Workspace_sptr ows = loadUB.getProperty("InputWorkspace");
    return boost::dynamic_pointer_cast<PeaksWorkspace>(ows);
  }

  PeaksWorkspace_sptr m_inputPeaksWS;
};

#endif /* OPTIMIZECRYSTALPLACEMENTTEST_H_ */
