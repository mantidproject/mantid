// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.hxx"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_const_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::DetectorInfo;
using Mantid::Kernel::IPropertyManager;

class GetDetectorOffsetsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetDetectorOffsetsTest *createSuite() { return new GetDetectorOffsetsTest(); }
  static void destroySuite(GetDetectorOffsetsTest *suite) { delete suite; }

  GetDetectorOffsetsTest() { Mantid::API::FrameworkManager::Instance(); }

  void testTheBasics() {
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    TS_ASSERT_EQUALS(offsets.name(), "GetDetectorOffsets");
    TS_ASSERT_EQUALS(offsets.version(), 1);
  }

  void testExec() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], -0.0196, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecWithGroup() {
    // --------- Workspace with summed spectra -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::createGroupedWorkspace2D(3, 200, 1.0);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->getValue(1), -0.0196, 0.0001);
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(2));
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(3));

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecAbsolute() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "10"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OffsetMode", "Absolute"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DIdeal", "3.5"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], 2.4803, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecSigned() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "10"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], -1.0, 0.01);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecFWHM() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("EstimateFWHM", "1"))
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], -0.11074, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecAbsoluteFWHM() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "10"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OffsetMode", "Absolute"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DIdeal", "3.5"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("EstimateFWHM", "1"))
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], 2.38925, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  void testExecSignedFWHM() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", outputWS + "_mask"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "10"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OffsetMode", "Signed"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("EstimateFWHM", "1"))
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWS));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], -6.22677, 0.01);

    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove(outputWS + "_mask");
  }

  /*
   * Set mask flags using spectrum number.
   */
  void maskAllDetectorsInSpectrum(MaskWorkspace_sptr mask, MatrixWorkspace_const_sptr input, size_t ns) {
    for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
      mask->setMasked(det, true);
    }
  }

  /*
   * A shared initialization stage for all of the mask tests.
   */
  auto maskTestInitilization(IPropertyManager &algorithmProperties, const std::string &uniquePrefix) {
    using Mantid::HistogramData::Counts;
    using Mantid::HistogramData::CountStandardDeviations;
    using Mantid::HistogramData::Histogram;
    using Mantid::HistogramData::Points;
    using Histogram_sptr = std::shared_ptr<Histogram>;

    const std::string maskWSName(uniquePrefix + "_mask");

    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("XMin", "-20.0"));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("XMax", "20.0"));
    //    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("MaxOffset", "20.0"));

    std::function<Histogram_sptr(double, double, std::size_t, double, double)> peakFunc =
        [](double x_0, double x_1, std::size_t N_x, double p_0, double p_width) -> Histogram_sptr {
      const double dx = (x_1 - x_0) / (double(N_x - 1));
      Histogram_sptr rval = std::make_shared<Histogram>(Histogram::XMode::Points, Histogram::YMode::Counts);
      rval->resize(N_x); // resizes x: Points
      rval->setCounts(Counts(N_x));
      rval->setCountStandardDeviations(CountStandardDeviations(N_x));

      auto &vx = rval->mutableX();
      auto &vy = rval->mutableY();
      auto &ve = rval->mutableE();
      double x = x_0;
      for (std::size_t n = 0; n < N_x; ++n, x += dx) {
        vx[n] = x;
        vy[n] = exp(-0.5 * pow((x - p_0) / p_width, 2));
        ve[n] = sqrt(vy[n]);
      }
      return rval;
    };
    std::vector<double> expectedPeakFitLocations = {-0.0909090909, -0.1071428571, -0.1228070175, -0.1379310344,
                                                    -0.1525423728, -0.1666666666, -0.1803278688, -0.1935483870,
                                                    -0.2063492063, -0.2187499999};

    Workspace2D_sptr input = WorkspaceCreationHelper::create2DWorkspaceFromFunctionAndArgsList(
        peakFunc, {
                      // note that "{0.0, 200.0, std::size_t(200), 1.0, 1.0}" reliably fails
                      {0.0, 200.0, std::size_t(200), 5.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 6.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 7.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 8.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 9.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 10.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 11.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 12.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 13.0, 5.0},
                      {0.0, 200.0, std::size_t(200), 14.0, 5.0},
                  });

    input->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*input, false, false, "test instrument");

    MaskWorkspace_sptr mask = std::make_shared<MaskWorkspace>(input->getInstrument());
    AnalysisDataService::Instance().add(maskWSName, mask);

    Histogram_sptr exampleFailingSpectrum = peakFunc(0.0, 200.0, std::size_t(200), 1.0, 1.0);

    return std::make_tuple(input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum);
  }
  /**
   * Verify that the optional mask workspace input parameter is properly treated:
   *   -- parameter not present: default mask is created and exists in the ADS.
   */
  void testMaskIsCreated() {
    const std::string uniquePrefix = "test_MIC";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);

    // Make sure any incoming mask workspace doesn't exist.
    AnalysisDataService::Instance().remove(maskWSName);
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(maskWSName));

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MaskWorkspace_const_sptr outputMask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(outputMask);
    TS_ASSERT_EQUALS(outputMask->getNumberMasked(), 0);

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  /**
   * Verify that the optional mask workspace input parameter is properly treated:
   *   -- parameter is present: default mask should not be created.
   */
  void testInputMaskIsUsed() {
    const std::string uniquePrefix = "test_IMIU";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);

    TS_ASSERT(AnalysisDataService::Instance().doesExist(maskWSName));
    // Set the mask-workspace title to a random string:
    const std::string maskWSTitle("f09ab021-8101-4720-bdc5-5758fc8be4f6");
    MaskWorkspace_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    mask->setTitle(maskWSTitle);
    TS_ASSERT(mask->getTitle() == maskWSTitle);

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);
    TS_ASSERT(mask->getTitle() == maskWSTitle);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), 0);

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  // Verify that all of the input spectra used by the masking tests succeed during the peak-fitting step.
  void testExecNoneAreMasked() {
    const std::string uniquePrefix = "test_ENAM";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);
    MaskWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);

    size_t ns(0);
    for (auto it = expectedPeakFitLocations.cbegin(); it != expectedPeakFitLocations.cend(); ++it, ++ns) {
      for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
        TS_ASSERT(!mask->isMasked(det));
        TS_ASSERT_DELTA(output->getValue(det), *it, 0.001);
      }
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  // Verify that any detectors which are already masked in the input are still masked in the output,
  //   regardless of whether or not peaks can be fitted to their associated spectra.
  void testExecMaskedStayMasked() {
    const std::string uniquePrefix = "test_EMSM";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);
    MaskWorkspace_sptr mask;
    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);

    maskAllDetectorsInSpectrum(mask, input, 3);
    maskAllDetectorsInSpectrum(mask, input, 6);
    maskAllDetectorsInSpectrum(mask, input, 7);
    std::vector<int> expectedMaskValues(input->getNumberHistograms(), 0);
    expectedMaskValues[3] = 1;
    expectedMaskValues[6] = 1;
    expectedMaskValues[7] = 1;

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);

    size_t ns(0);
    for (auto it = expectedPeakFitLocations.cbegin(); it != expectedPeakFitLocations.cend(); ++it, ++ns) {
      for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
        if (!mask->isMasked(det))
          TS_ASSERT_DELTA(output->getValue(det), *it, 0.001);
        TS_ASSERT(mask->isMasked(det) == (expectedMaskValues[ns] == 1));
      }
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  // Verify that detectors which fail during peak fitting are masked.
  void testExecFailuresAreMasked() {
    const std::string uniquePrefix = "test_EFAM";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);

    input->getSpectrum(0).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(5).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(9).setHistogram(*exampleFailingSpectrum);
    std::vector<int> expectedMaskValues(input->getNumberHistograms(), 0);
    expectedMaskValues[0] = 1;
    expectedMaskValues[5] = 1;
    expectedMaskValues[9] = 1;

    // ---- Run algo -----
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);
    MaskWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);

    size_t ns(0);
    for (auto it = expectedPeakFitLocations.cbegin(); it != expectedPeakFitLocations.cend(); ++it, ++ns) {
      for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
        if (!mask->isMasked(det))
          TS_ASSERT_DELTA(output->getValue(det), *it, 0.001);
        TS_ASSERT(mask->isMasked(det) == (expectedMaskValues[ns] == 1));
      }
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  // Verify that detectors masked in the output include both those masked in the input, and also those that fail during
  // peak fitting.
  void testExecMasksAreCombined() {
    const std::string uniquePrefix = "test_EFAM";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);
    MaskWorkspace_sptr mask;
    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);

    maskAllDetectorsInSpectrum(mask, input, 3);
    maskAllDetectorsInSpectrum(mask, input, 6);
    maskAllDetectorsInSpectrum(mask, input, 7);
    input->getSpectrum(0).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(5).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(9).setHistogram(*exampleFailingSpectrum);
    std::vector<int> expectedMaskValues(input->getNumberHistograms(), 0);
    expectedMaskValues[3] = 1;
    expectedMaskValues[6] = 1;
    expectedMaskValues[7] = 1;
    expectedMaskValues[0] = 1;
    expectedMaskValues[5] = 1;
    expectedMaskValues[9] = 1;

    // ---- Run algo -----
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);

    size_t ns(0);
    for (auto it = expectedPeakFitLocations.cbegin(); it != expectedPeakFitLocations.cend(); ++it, ++ns) {
      for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
        if (!mask->isMasked(det))
          TS_ASSERT_DELTA(output->getValue(det), *it, 0.001);
        TS_ASSERT(mask->isMasked(det) == (expectedMaskValues[ns] == 1));
      }
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }

  /**
   * Verify that the output offsets and mask workspaces are have detector mask flags which are consistent with the mask
   * values.
   */
  void testExecMasksAreConsistentWithDetectorFlags() {
    const std::string uniquePrefix = "test_EFAM";
    const std::string outputWSName(uniquePrefix + "_offsets");

    GetDetectorOffsets offsets;
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
    auto [input, maskWSName, expectedPeakFitLocations, exampleFailingSpectrum] =
        maskTestInitilization(offsets, uniquePrefix);
    MaskWorkspace_sptr mask;
    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);

    maskAllDetectorsInSpectrum(mask, input, 3);
    maskAllDetectorsInSpectrum(mask, input, 6);
    maskAllDetectorsInSpectrum(mask, input, 7);
    input->getSpectrum(0).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(5).setHistogram(*exampleFailingSpectrum);
    input->getSpectrum(9).setHistogram(*exampleFailingSpectrum);
    std::vector<int> expectedMaskValues(input->getNumberHistograms(), 0);
    expectedMaskValues[3] = 1;
    expectedMaskValues[6] = 1;
    expectedMaskValues[7] = 1;
    expectedMaskValues[0] = 1;
    expectedMaskValues[5] = 1;
    expectedMaskValues[9] = 1;

    // ---- Run algo -----
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", input));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);

    size_t ns(0);
    for (auto it = expectedPeakFitLocations.cbegin(); it != expectedPeakFitLocations.cend(); ++it, ++ns) {
      for (const auto &det : input->getSpectrum(ns).getDetectorIDs()) {
        if (!mask->isMasked(det))
          TS_ASSERT_DELTA(output->getValue(det), *it, 0.001);
        TS_ASSERT(mask->isMasked(det) == (expectedMaskValues[ns] == 1));
      }
    }

    TS_ASSERT(mask->isConsistentWithDetectorMasks());
    TS_ASSERT(mask->isConsistentWithDetectorMasks(output->detectorInfo()));

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }
};

class GetDetectorOffsetsTestPerformance : public CxxTest::TestSuite {
  MatrixWorkspace_sptr WS;
  int numpixels;

public:
  static GetDetectorOffsetsTestPerformance *createSuite() { return new GetDetectorOffsetsTestPerformance(); }
  static void destroySuite(GetDetectorOffsetsTestPerformance *suite) { delete suite; }

  GetDetectorOffsetsTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    numpixels = 10000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numpixels, 200, false);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {

      auto xvals = WS->points(wi);
      auto &Y = WS->mutableY(wi);

      std::transform(xvals.cbegin(), xvals.cend(), Y.begin(),
                     [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
      auto &E = WS->mutableE(wi);
      E.assign(E.size(), 0.001);
    }
  }

  void test_performance() {
    AlgorithmManager::Instance(); // Initialize here to avoid an odd ABORT
    GetDetectorOffsets offsets;
    if (!offsets.isInitialized())
      offsets.initialize();

    const std::string outputWSName("offsetsped");
    const std::string maskWSName(outputWSName + "_mask");

    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outputWSName));
    TS_ASSERT(output);

    TS_ASSERT_DELTA(output->y(0)[0], -0.0196, 0.0001);

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(maskWSName);
  }
};
