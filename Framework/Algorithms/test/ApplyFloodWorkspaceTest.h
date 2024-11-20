// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ApplyFloodWorkspace.h"
#include "MantidAlgorithms/CropWorkspace.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

namespace {
constexpr double MAX_X_REFLECTOMETRY(100000);
constexpr double DELTA(1e-9);

std::vector<double> generateXRange(const int nBoundaries, double start) {
  auto xvec = std::vector<double>(nBoundaries + 1);
  double const interval = MAX_X_REFLECTOMETRY / nBoundaries; // increment
  std::ranges::for_each(xvec.begin(), xvec.end(), [&start, &interval](double &item) {
    item = start;
    start += interval;
  });
  return xvec;
}

MatrixWorkspace_sptr cropWorkspaces(const MatrixWorkspace_sptr &inputWS, const int startIndex = 0) {
  CropWorkspace alg;
  alg.initialize();
  alg.setChild(true);
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("StartWorkspaceIndex", startIndex);
  alg.setProperty("OutputWorkspace", "dummy");
  alg.execute();
  MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
  return out;
}

MatrixWorkspace_sptr applyFloodWorkspace(const MatrixWorkspace_sptr &inputWS, const MatrixWorkspace_sptr &floodWS) {
  ApplyFloodWorkspace alg;
  alg.initialize();
  alg.setChild(true);
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("FloodWorkspace", floodWS);
  alg.setProperty("OutputWorkspace", "dummy");
  alg.execute();
  return alg.getProperty("OutputWorkspace");
}
} // namespace

class ApplyFloodWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_flood_same_x_units() {
    auto inputWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument());

    auto const out = applyFloodWorkspace(inputWS, flood);
    TS_ASSERT_DELTA(out->y(0)[0], 2.8571428575, DELTA);
    TS_ASSERT_DELTA(out->y(1)[0], 2.0, DELTA);
    TS_ASSERT_DELTA(out->y(2)[0], 2.5, DELTA);
    TS_ASSERT_DELTA(out->y(3)[0], 2.2222222222, DELTA);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_different_x_units() {
    auto inputWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument(), "Wavelength");

    auto const out = applyFloodWorkspace(inputWS, flood);
    TS_ASSERT_DELTA(out->y(0)[0], 2.8571428575, DELTA);
    TS_ASSERT_DELTA(out->y(1)[0], 2.0, DELTA);
    TS_ASSERT_DELTA(out->y(2)[0], 2.5, DELTA);
    TS_ASSERT_DELTA(out->y(3)[0], 2.2222222222, DELTA);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_doesnt_transform_spectra_that_are_missing_in_flood_workspace_for_multiple_bin_file() {
    auto inputWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument(), "TOF", 4);

    auto const cropped = cropWorkspaces(flood, 2);
    auto const out = applyFloodWorkspace(inputWS, cropped);

    // Histograms without Flood Spectra data are not modified
    TS_ASSERT_DELTA(out->readY(0), inputWS->readY(0), DELTA);
    TS_ASSERT_DELTA(out->readY(1), inputWS->readY(1), DELTA);

    int const refactor = 5; // how bigger is bin size in input with respect to initial flood ws
    // Histograms with Flood Spectra are rebinned prior to flood correction
    TS_ASSERT_DELTA(out->readY(2), std::vector<double>(20, 2 / 0.8 * refactor), DELTA);
    TS_ASSERT_DELTA(out->readY(3), std::vector<double>(20, 2 / 0.9 * refactor), DELTA);
    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr createFloodWorkspace(const Mantid::Geometry::Instrument_const_sptr &instrument,
                                            std::string const &xUnit = "TOF", const int nBoundaries = 1) {
    MatrixWorkspace_sptr flood = create2DWorkspace(4, nBoundaries);
    flood->mutableY(0) = std::vector<double>(nBoundaries, 0.7);
    flood->mutableY(1) = std::vector<double>(nBoundaries, 1.0);
    flood->mutableY(2) = std::vector<double>(nBoundaries, 0.8);
    flood->mutableY(3) = std::vector<double>(nBoundaries, 0.9);

    // X values are added ad-hoc to match the bin X range of the default test Reflectometry Instrument workspaces
    //  This doesn't affect tests with 1 bin per histogram
    auto const &xvec = generateXRange(nBoundaries, 0);
    flood->mutableX(0) = xvec;
    flood->mutableX(1) = xvec;
    flood->mutableX(2) = xvec;
    flood->mutableX(3) = xvec;

    flood->setInstrument(instrument);
    for (size_t i = 0; i < flood->getNumberHistograms(); ++i) {
      flood->getSpectrum(i).setDetectorID(Mantid::detid_t(i + 1));
    }
    flood->getAxis(0)->setUnit("TOF");
    if (xUnit != "TOF") {
      ConvertUnits convert;
      convert.initialize();
      convert.setChild(true);
      convert.setProperty("InputWorkspace", flood);
      convert.setProperty("Target", xUnit);
      convert.setProperty("OutputWorkspace", "dummy");
      convert.execute();
      flood = convert.getProperty("OutputWorkspace");
    }
    return flood;
  }
};
