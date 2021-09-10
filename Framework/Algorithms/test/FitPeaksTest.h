// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::FitPeaks;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;

class FitPeaksTest : public CxxTest::TestSuite {
private:
  std::string m_inputWorkspaceName{"FitPeaksTest_workspace"};

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeaksTest *createSuite() { return new FitPeaksTest(); }
  static void destroySuite(FitPeaksTest *suite) { delete suite; }

  void setUp() override {
    // Needs other algorithms and functions to be registered
    FrameworkManager::Instance();
  }

  void test_Init() {
    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** test fit a single peak in partial spectra from a multiple spectra
   * workspace
   * the peak positions are given by the peak position workspace and thus the
   * peak fit windows
   * @brief test_singlePeaksPartialSpectra
   */
  void test_singlePeaksPartialSpectra() {
    // Generate input workspace
    const std::string data_ws_name("Test1Data");
    genearteTestDataGaussian(data_ws_name);

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createBackToBackExponentialParameters(peakparnames, peakparvalues);

    // create a 1-value peak index vector for peak (0) at X=5
    std::vector<int> peak_index_vec;
    peak_index_vec.emplace_back(0);
    const std::string ws_name("peakcenter1");
    const std::string peak_center_ws_name = genPeakCenterWorkspace(peak_index_vec, ws_name);
    const std::string fit_window_ws_name = genFitWindowWorkspace(peak_index_vec, "peakwindow1");

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", data_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 1));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCentersWorkspace", peak_center_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitPeakWindowWorkspace", fit_window_ws_name))

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS3");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS3");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS3");
    fitpeaks.setProperty("MaxFitIterations", 200);

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());
    if (fitpeaks.isExecuted()) {
      // check output workspaces
      TS_ASSERT(API::AnalysisDataService::Instance().doesExist("PeakPositionsWS3"));
      TS_ASSERT(API::AnalysisDataService::Instance().doesExist("PeakParametersWS3"));
      TS_ASSERT(API::AnalysisDataService::Instance().doesExist("FittedPeaksWS3"));

      // about the parameters
      API::MatrixWorkspace_sptr peak_params_ws =
          std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PeakPositionsWS3"));
      TS_ASSERT(peak_params_ws);
      // 2 spectra
      TS_ASSERT_EQUALS(peak_params_ws->getNumberHistograms(), 2);
      // 1 peak
      TS_ASSERT_EQUALS(peak_params_ws->histogram(0).x().size(), 1);

      // clean algorithm-generated workspaces
      API::AnalysisDataService::Instance().remove("PeakPositionsWS3");
      API::AnalysisDataService::Instance().remove("PeakParametersWS3");
      API::AnalysisDataService::Instance().remove("FittedPeaksWS3");
    }

    // clean
    API::AnalysisDataService::Instance().remove(fit_window_ws_name);
    API::AnalysisDataService::Instance().remove(peak_center_ws_name);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**
   * @brief test_multiPeaksMultiSpectra
   */
  void test_multiPeaksMultiSpectra() {
    // run serially so values don't depend on no. cores etc.
    FrameworkManager::Instance().setNumOMPThreads(1);

    // set up parameters with starting value
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createGuassParameters(peakparnames, peakparvalues);

    // Generate input workspace
    genearteTestDataGaussian(m_inputWorkspaceName);

    // initialize algorithm to test
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 2));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "5.0, 10.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowBoundaryList", "2.5, 6.5, 8.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS");
    fitpeaks.setProperty("ConstrainPeakPositions", false);

    fitpeaks.execute();

    // check result
    TS_ASSERT(fitpeaks.isExecuted());
    if (!fitpeaks.isExecuted())
      return;

    // get fitted peak data
    API::MatrixWorkspace_sptr main_out_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PeakPositionsWS"));
    TS_ASSERT(main_out_ws);
    TS_ASSERT_EQUALS(main_out_ws->getNumberHistograms(), 3);

    API::MatrixWorkspace_sptr plot_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("FittedPeaksWS"));
    TS_ASSERT(plot_ws);
    TS_ASSERT_EQUALS(plot_ws->getNumberHistograms(), 3);

    API::ITableWorkspace_sptr param_ws =
        std::dynamic_pointer_cast<API::ITableWorkspace>(AnalysisDataService::Instance().retrieve("PeakParametersWS"));
    TS_ASSERT(param_ws);
    TS_ASSERT_EQUALS(param_ws->rowCount(), 6);

    // check values: fitted peak positions
    // spectrum 1
    const auto &fitted_positions_0 = main_out_ws->histogram(0).y();
    TS_ASSERT_EQUALS(fitted_positions_0.size(), 2); // with 2 peaks to fit
    TS_ASSERT_DELTA(fitted_positions_0[0], 5.0, 1.E-6);
    TS_ASSERT_DELTA(fitted_positions_0[1], 10.0, 1.E-6);
    // spectrum 3
    const auto &fitted_positions_2 = main_out_ws->histogram(2).y();
    TS_ASSERT_EQUALS(fitted_positions_2.size(), 2); // with 2 peaks to fit
    TS_ASSERT_DELTA(fitted_positions_2[0], 5.03, 1.E-6);
    TS_ASSERT_DELTA(fitted_positions_2[1], 10.02, 1.E-6);

    // check other fitted parameters including height and width
    // spectrum 2
    double ws1peak0_height = param_ws->cell<double>(2, 2);
    double ws1peak0_width = param_ws->cell<double>(2, 4);
    TS_ASSERT_DELTA(ws1peak0_height, 4., 1E-6);
    TS_ASSERT_DELTA(ws1peak0_width, 0.17, 1E-6);
    double ws1peak1_height = param_ws->cell<double>(3, 2);
    double ws1peak1_width = param_ws->cell<double>(3, 4);
    TS_ASSERT_DELTA(ws1peak1_height, 2., 1E-6);
    TS_ASSERT_DELTA(ws1peak1_width, 0.12, 1E-6);

    // check the fitted peak workspace
    API::MatrixWorkspace_sptr data_ws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve(m_inputWorkspaceName));
    TS_ASSERT_EQUALS(plot_ws->histogram(0).x().size(), data_ws->histogram(0).x().size());
    TS_ASSERT_DELTA(plot_ws->histogram(0).x().front(), data_ws->histogram(0).x().front(), 1E-10);
    TS_ASSERT_DELTA(plot_ws->histogram(0).x().back(), data_ws->histogram(0).x().back(), 1E-10);

    // clean up
    AnalysisDataService::Instance().remove(m_inputWorkspaceName);
    AnalysisDataService::Instance().remove("PeakPositionsWS");
    AnalysisDataService::Instance().remove("FittedPeaksWS");
    AnalysisDataService::Instance().remove("PeakParametersWS");

    FrameworkManager::Instance().setNumOMPThreadsToConfigValue();
  }

  //----------------------------------------------------------------------------------------------
  /** Test output of effective peak parameters
   * @brief test_effectivePeakParameters
   */
  void test_effectivePeakParameters() {
    // run serially so values don't depend on no. cores etc.
    FrameworkManager::Instance().setNumOMPThreads(1);

    // set up parameters with starting value
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createGuassParameters(peakparnames, peakparvalues);

    // Generate input workspace
    genearteTestDataGaussian(m_inputWorkspaceName);

    // initialize algorithm to test
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 2));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "5.0, 10.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowBoundaryList", "2.5, 6.5, 8.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("RawPeakParameters", false));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS");
    fitpeaks.setProperty("ConstrainPeakPositions", false);

    fitpeaks.execute();

    // check result
    TS_ASSERT(fitpeaks.isExecuted());
    if (!fitpeaks.isExecuted())
      return;

    // get fitted peak data
    API::MatrixWorkspace_sptr main_out_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PeakPositionsWS"));
    TS_ASSERT(main_out_ws);
    TS_ASSERT_EQUALS(main_out_ws->getNumberHistograms(), 3);

    API::MatrixWorkspace_sptr plot_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("FittedPeaksWS"));
    TS_ASSERT(plot_ws);
    TS_ASSERT_EQUALS(plot_ws->getNumberHistograms(), 3);

    API::ITableWorkspace_sptr param_ws =
        std::dynamic_pointer_cast<API::ITableWorkspace>(AnalysisDataService::Instance().retrieve("PeakParametersWS"));
    TS_ASSERT(param_ws);
    TS_ASSERT_EQUALS(param_ws->rowCount(), 6);

    // check values: fitted peak positions
    // spectrum 1
    const auto &fitted_positions_0 = main_out_ws->histogram(0).y();
    TS_ASSERT_EQUALS(fitted_positions_0.size(), 2); // with 2 peaks to fit
    TS_ASSERT_DELTA(fitted_positions_0[0], 5.0, 1.E-6);
    TS_ASSERT_DELTA(fitted_positions_0[1], 10.0, 1.E-6);
    // spectrum 3
    const auto &fitted_positions_2 = main_out_ws->histogram(2).y();
    TS_ASSERT_EQUALS(fitted_positions_2.size(), 2); // with 2 peaks to fit
    TS_ASSERT_DELTA(fitted_positions_2[0], 5.03, 1.E-6);
    TS_ASSERT_DELTA(fitted_positions_2[1], 10.02, 1.E-6);

    // check other fitted parameters including height and width
    // spectrum 2: (center, width, height, intensity)
    double ws1peak0_height = param_ws->cell<double>(2, 4);
    double ws1peak0_width = param_ws->cell<double>(2, 3);
    TS_ASSERT_DELTA(ws1peak0_height, 4., 1E-6);
    TS_ASSERT_DELTA(ws1peak0_width, 0.17 * 2.3548, 1E-4);

    double ws1peak1_height = param_ws->cell<double>(3, 4);
    double ws1peak1_width = param_ws->cell<double>(3, 3);
    TS_ASSERT_DELTA(ws1peak1_height, 2., 1E-6);
    TS_ASSERT_DELTA(ws1peak1_width, 0.12 * 2.3548, 1E-4);

    // check the fitted peak workspace
    API::MatrixWorkspace_sptr data_ws = std::dynamic_pointer_cast<API::MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve(m_inputWorkspaceName));
    TS_ASSERT_EQUALS(plot_ws->histogram(0).x().size(), data_ws->histogram(0).x().size());
    TS_ASSERT_DELTA(plot_ws->histogram(0).x().front(), data_ws->histogram(0).x().front(), 1E-10);
    TS_ASSERT_DELTA(plot_ws->histogram(0).x().back(), data_ws->histogram(0).x().back(), 1E-10);

    // clean up
    AnalysisDataService::Instance().remove(m_inputWorkspaceName);
    AnalysisDataService::Instance().remove("PeakPositionsWS");
    AnalysisDataService::Instance().remove("FittedPeaksWS");
    AnalysisDataService::Instance().remove("PeakParametersWS");

    FrameworkManager::Instance().setNumOMPThreadsToConfigValue();
  }

  //----------------------------------------------------------------------------------------------
  /** Test a subset of spectra that do not have any count
   * Thus, 2 main features of algorithm FitPeaks will be examed here
   * 1. partial spectra
   * 2. no signal with event count workspace
   * @brief test_NoSignaleWorkspace2D
   */
  void test_NoSignaleWorkspace2D() {
    // load file to workspace
    std::string input_ws_name("PG3_733");

    // Start by loading our NXS file
    auto loader = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
    loader->setPropertyValue("Filename", "PG3_733.nxs");
    loader->setPropertyValue("OutputWorkspace", input_ws_name);
    loader->execute();
    TS_ASSERT(loader->isExecuted());

    // Initialize FitPeak
    FitPeaks fit_peaks_alg;

    fit_peaks_alg.initialize();
    TS_ASSERT(fit_peaks_alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("InputWorkspace", input_ws_name));

    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("PeakCenters",
                                                       "0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0."
                                                       "6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,"
                                                       "1.5133,2.1401"));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("StartWorkspaceIndex", 3));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("StopWorkspaceIndex", 3));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("FitFromRight", false));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("HighBackground", true));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("PeakWidthPercent", 0.016)); // typical powgen's

    std::string peak_pos_ws_name("PG3_733_peak_positions");
    std::string peak_param_ws_name("PG3_733_peak_params");
    fit_peaks_alg.setProperty("OutputWorkspace", peak_pos_ws_name);
    fit_peaks_alg.setProperty("OutputPeakParametersWorkspace", peak_param_ws_name);

    fit_peaks_alg.execute();
    TS_ASSERT(fit_peaks_alg.isExecuted());
    if (!fit_peaks_alg.isExecuted())
      return;

    // get result
    bool peak_pos_ws_exist, peak_param_ws_exist;
    API::MatrixWorkspace_sptr peak_pos_ws = CheckAndRetrieveMatrixWorkspace(peak_pos_ws_name, &peak_pos_ws_exist);
    API::ITableWorkspace_sptr peak_param_ws = CheckAndRetrieveTableWorkspace(peak_param_ws_name, &peak_param_ws_exist);

    // fitted peak position workspace.  should contain 1 spectrum for workspace
    // index 3
    if (peak_pos_ws_exist) {
      TS_ASSERT_EQUALS(peak_pos_ws->getNumberHistograms(), 1);
      HistogramData::HistogramX hist_x = peak_pos_ws->histogram(0).x();
      HistogramData::HistogramY hist_y = peak_pos_ws->histogram(0).y();
      TS_ASSERT_EQUALS(hist_y.size(), 17);
      TS_ASSERT_DELTA(hist_x[0], 0.5044, 1.E-12);
      TS_ASSERT_DELTA(hist_y[0], -1., 1.E-12);
    }

    if (peak_param_ws_exist) {
      TS_ASSERT_EQUALS(peak_param_ws->rowCount(), 17);
    }

    if (!peak_pos_ws_exist || !peak_param_ws_exist)
      return;

    // clean up
    AnalysisDataService::Instance().remove("PG3_733");
    AnalysisDataService::Instance().remove("PG3_733_EventNumbers");
    AnalysisDataService::Instance().remove(peak_pos_ws_name);
    AnalysisDataService::Instance().remove("PG3_733_peak_params");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test fit Gaussian peaks with high background
   * @brief Later_test_HighBackgroundPeaks
   */
  void test_HighBackgroundPeaks() {
    // load file to workspace
    std::string input_ws_name("PG3_733");

    // Start by loading our NXS file
    auto loader = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
    loader->setPropertyValue("Filename", "PG3_733.nxs");
    loader->setPropertyValue("OutputWorkspace", input_ws_name);
    loader->execute();
    TS_ASSERT(loader->isExecuted());

    // Initialize FitPeak
    FitPeaks fit_peaks_alg;

    fit_peaks_alg.initialize();
    TS_ASSERT(fit_peaks_alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("InputWorkspace", input_ws_name));

    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("PeakCenters",
                                                       "0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1."
                                                       "2356, 1.5133, 2.1401"));
    fit_peaks_alg.setProperty("StartWorkspaceIndex", 0);
    fit_peaks_alg.setProperty("StopWorkspaceIndex", 3);
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("HighBackground", true));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("PeakWidthPercent", 0.016)); // typical powgen's

    std::string output_ws_name("PG3_733_stripped");
    std::string peak_pos_ws_name("PG3_733_peak_positions");
    std::string peak_param_ws_name("PG3_733_peak_params");
    fit_peaks_alg.setProperty("OutputWorkspace", peak_pos_ws_name);
    fit_peaks_alg.setProperty("OutputPeakParametersWorkspace", peak_param_ws_name);
    fit_peaks_alg.setProperty("FittedPeaksWorkspace", output_ws_name);

    fit_peaks_alg.execute();
    TS_ASSERT(fit_peaks_alg.isExecuted());
    if (!fit_peaks_alg.isExecuted())
      return;

    // Check result
    bool peak_pos_ws_exist, peak_param_ws_exist, fitted_peak_ws_exist;
    API::MatrixWorkspace_sptr peak_pos_ws = CheckAndRetrieveMatrixWorkspace(peak_pos_ws_name, &peak_pos_ws_exist);
    API::MatrixWorkspace_sptr fitted_peak_ws = CheckAndRetrieveMatrixWorkspace(output_ws_name, &fitted_peak_ws_exist);
    API::ITableWorkspace_sptr peak_param_ws = CheckAndRetrieveTableWorkspace(peak_param_ws_name, &peak_param_ws_exist);

    // check peak positions
    if (peak_pos_ws_exist) {
      TS_ASSERT_EQUALS(peak_pos_ws->getNumberHistograms(), 4);
      TS_ASSERT_EQUALS(peak_pos_ws->histogram(0).size(), 10);
      TS_ASSERT_DELTA(peak_pos_ws->histogram(0).y().back(), 2.1483553, 0.0005);
    }

    if (fitted_peak_ws_exist) {
      TS_ASSERT_EQUALS(fitted_peak_ws->getNumberHistograms(), 4);
    }

    if (peak_param_ws_exist) {
      TS_ASSERT_EQUALS(peak_param_ws->rowCount(), 40);
      TS_ASSERT_EQUALS(peak_param_ws->columnCount(), 9);
      TS_ASSERT_EQUALS(peak_param_ws->cell<int>(10, 0), 1);
      TS_ASSERT_EQUALS(peak_param_ws->cell<int>(22, 1), 2);

      // check first peak's height, center and sigma
      TS_ASSERT_DELTA(peak_param_ws->cell<double>(9, 2), 414.48, 10.0);
      TS_ASSERT_DELTA(peak_param_ws->cell<double>(9, 3), 2.14836, 0.0006);
      TS_ASSERT_DELTA(peak_param_ws->cell<double>(9, 4), 0.005051, 0.0005);
    }

    // Clean up
    AnalysisDataService::Instance().remove(input_ws_name);
    AnalysisDataService::Instance().remove(output_ws_name);
    AnalysisDataService::Instance().remove(peak_pos_ws_name);
    AnalysisDataService::Instance().remove(peak_param_ws_name);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test fitting Back-to-back exponential with single peak on multiple spectra
   * Test includes
   * 1. fit high angle detector spectra from 7 to 15 (StopWorkspace is included)
   * 2. fit 1 peak at d = 1.0758
   */
  void test_singlePeakMultiSpectraBackToBackExp() {
    // Generate input workspace
    std::string input_ws_name = generateTestDataBackToBackExponential();
    // Specify output workspaces names
    std::string peak_pos_ws_name("PeakPositionsB2BsPmS");
    std::string param_ws_name("PeakParametersB2BsPmS");
    std::string model_ws_name("ModelB2BsPmS");

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createBackToBackExponentialParameters(peakparnames, peakparvalues, false);

    // Input data workspace contains 16 spectra
    auto inputWS =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(input_ws_name));
    TS_ASSERT(inputWS);
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), 16);

    size_t start_ws_index = 7;
    size_t stop_ws_index = 15;

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", input_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", static_cast<int>(start_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", static_cast<int>(stop_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakFunction", "BackToBackExponential"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "1.0758"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowBoundaryList", "1.05, 1.11"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));

    fitpeaks.setProperty("OutputWorkspace", peak_pos_ws_name);
    fitpeaks.setProperty("OutputPeakParametersWorkspace", param_ws_name);
    fitpeaks.setProperty("FittedPeaksWorkspace", model_ws_name);

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    // check output workspaces
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist(peak_pos_ws_name));
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist(param_ws_name));
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist(model_ws_name));

    // About the parameters
    API::MatrixWorkspace_sptr peak_pos_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(peak_pos_ws_name));
    // Check peak values
    // Only the number of selected spectra will be recorded
    size_t expectedNumSpectra = stop_ws_index - start_ws_index + 1;

    TS_ASSERT(peak_pos_ws);
    TS_ASSERT_EQUALS(peak_pos_ws->getNumberHistograms(), expectedNumSpectra);
    TS_ASSERT_EQUALS(peak_pos_ws->histogram(0).y().size(), 1);
    // verify fitted peak position values
    for (size_t i = 0; i < expectedNumSpectra; ++i) {
      TS_ASSERT_DELTA(peak_pos_ws->histogram(i).y()[0], 1.0758, 1.2E-3);
    }

    // Get the table workspace
    auto param_table =
        std::dynamic_pointer_cast<API::ITableWorkspace>(AnalysisDataService::Instance().retrieve(param_ws_name));
    TS_ASSERT(param_table);

    // Columns: 0. wsindex, 1. peakindex, 2. I, 3. A, 4. B, 5. X0, 6. S, 7.
    // A0, 8. A1, 9. chi2
    std::vector<std::string> colnames = param_table->getColumnNames();
    TS_ASSERT_EQUALS(colnames.size(), 10);

    // Verify intensity: between 90 and 150
    for (size_t i = 0; i < expectedNumSpectra; ++i) {
      TS_ASSERT(param_table->cell<double>(i, 2) > 90. && param_table->cell<double>(i, 2) < 150);
    }
    // workspace index 10, 11 and 12 are smallest with output index at 4, 5 and
    // 6
    TS_ASSERT(param_table->cell<double>(0, 2) > param_table->cell<double>(4, 2));
    TS_ASSERT(param_table->cell<double>(8, 2) > param_table->cell<double>(5, 2));

    // Clean up
    AnalysisDataService::Instance().remove(input_ws_name);
    AnalysisDataService::Instance().remove(peak_pos_ws_name);
    AnalysisDataService::Instance().remove(param_ws_name);
    AnalysisDataService::Instance().remove(model_ws_name);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to fit multiple peaks on multiple spectra with back-to-back
   * exponential convoluted with Gaussian
   */
  void test_multiPeaksMultiSpectraBackToBackExp() {
    // Generate input workspace
    std::string input_ws_name = generateTestDataBackToBackExponential();
    API::MatrixWorkspace_sptr input_ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(input_ws_name));

    // Specify output workspaces names
    std::string peak_pos_ws_name("PeakPositionsB2BmPmS");
    std::string param_ws_name("PeakParametersB2BmPmS");
    std::string model_ws_name("ModelB2BmPmS");

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createBackToBackExponentialParameters(peakparnames, peakparvalues);

    size_t min_ws_index = 7;
    size_t max_ws_index = 15;

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", input_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", static_cast<int>(min_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", static_cast<int>(max_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakFunction", "BackToBackExponential"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "0.728299, 0.817, 0.89198, 1.0758"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowBoundaryList", "0.71, 0.76, 0.80, 0.84, 0.87, 0.91, 1.05, 1.11"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));

    fitpeaks.setProperty("OutputWorkspace", peak_pos_ws_name);
    fitpeaks.setProperty("OutputPeakParametersWorkspace", param_ws_name);
    fitpeaks.setProperty("FittedPeaksWorkspace", model_ws_name);

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    // Verify the existence of output workspaces
    bool peak_pos_ws_exist, peak_param_ws_exist, fitted_ws_exist;
    API::MatrixWorkspace_sptr peak_pos_ws = CheckAndRetrieveMatrixWorkspace(peak_pos_ws_name, &peak_pos_ws_exist);
    API::MatrixWorkspace_sptr model_ws = CheckAndRetrieveMatrixWorkspace(model_ws_name, &fitted_ws_exist);
    API::ITableWorkspace_sptr peak_param_ws = CheckAndRetrieveTableWorkspace(param_ws_name, &peak_param_ws_exist);

    size_t num_histograms = max_ws_index - min_ws_index + 1;

    // Verify peaks' positions
    if (peak_pos_ws_exist) {
      // workspace for peak positions from fitted value: must be a 16 x 4
      // workspace2D
      TS_ASSERT_EQUALS(peak_pos_ws->getNumberHistograms(), num_histograms);
      TS_ASSERT_EQUALS(peak_pos_ws->histogram(0).y().size(), 4);

      // Expected value: with one peak that cannot be fit
      std::vector<double> exp_positions{-4, 0.81854, 0.89198, 1.0758};

      for (size_t ih = 0; ih < num_histograms; ++ih) {
        // Get histogram
        HistogramData::HistogramY peak_pos_i = peak_pos_ws->histogram(ih).y();
        HistogramData::HistogramE pos_error_i = peak_pos_ws->histogram(ih).e();
        // Check value
        for (auto ip = 0; ip < 4; ++ip) {

          // FIXME - the exact reason why only this peak gets failed with
          // fitting has not been discovered yet.  It is related to the starting
          // values and fit window. Hopefully the future improved fitting
          // algorithm will solve this issue.
          if (ih == 5 && ip == 2) {
            continue;
          }

          // Check fitted peak positions with the expected positions
          TS_ASSERT_DELTA(peak_pos_i[ip], exp_positions[ip], 0.0012);
          if (peak_pos_i[ip] > 0) {
            // a good fit: require error less than 100
            TS_ASSERT(pos_error_i[ip] < 150.);
          } else {
            // failed fit
            TS_ASSERT(pos_error_i[ip] > 1E20);
          }
        }
      }
    }

    // Verify calcualated model
    if (fitted_ws_exist) {
      // workspace for calculated peaks from fitted data
      TS_ASSERT_EQUALS(model_ws->getNumberHistograms(), input_ws->getNumberHistograms());
    }

    if (peak_param_ws_exist) {
      // workspace for calcualted peak parameters
      TS_ASSERT_EQUALS(peak_param_ws->rowCount(), (4 * num_histograms));
      // check third spectrum
      size_t iws = 2;
      // peak is out of range.  zero intensity
      double peak_intensity_2_0 = peak_param_ws->cell<double>(iws * 4, 2);
      TS_ASSERT_DELTA(peak_intensity_2_0, 0., 1.E-20);
      double peak_intensity_2_2 = peak_param_ws->cell<double>(iws * 4 + 2, 2);
      TS_ASSERT_DELTA(peak_intensity_2_2, 46.77, 2.0);
      double peak_intensity_2_3 = peak_param_ws->cell<double>(iws * 4 + 3, 2);
      TS_ASSERT_DELTA(peak_intensity_2_3, 146.44, 2.0);
    }

    // clean
    AnalysisDataService::Instance().remove(input_ws_name);
    AnalysisDataService::Instance().remove(peak_pos_ws_name);
    AnalysisDataService::Instance().remove(param_ws_name);
    AnalysisDataService::Instance().remove(model_ws_name);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test to fit multiple peaks on multiple spectra with back-to-back
   * exponential convoluted with Gaussian. This test loads the Parameters.xml
   * file and guesses the parameters for A,B rather than providing them
   */
  void test_multiPeaksMultiSpectraBackToBackExp_with_Param_xml() {
    // run serially so values don't depend on no. cores etc.
    FrameworkManager::Instance().setNumOMPThreads(1);

    // Generate input workspace
    std::string input_ws_name = generateTestDataBackToBackExponential();

    // Load VULCAN_Parameters.xml
    auto pLoaderPF = AlgorithmManager::Instance().create("LoadParameterFile");
    pLoaderPF->initialize();
    pLoaderPF->setPropertyValue("Filename", "VULCAN_Parameters.xml");
    pLoaderPF->setPropertyValue("Workspace", input_ws_name);
    pLoaderPF->execute();

    API::MatrixWorkspace_sptr input_ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(input_ws_name));

    // Specify output workspaces names
    std::string peak_pos_ws_name("PeakPositionsB2BmPmS");
    std::string param_ws_name("PeakParametersB2BmPmS");
    std::string model_ws_name("ModelB2BmPmS");

    size_t min_ws_index = 7;
    size_t max_ws_index = 15;

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", input_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", static_cast<int>(min_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", static_cast<int>(max_ws_index)));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakFunction", "BackToBackExponential"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("BackgroundType", "Linear"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "0.728299, 0.817, 0.89198, 1.0758"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowBoundaryList", "0.71, 0.76, 0.80, 0.84, 0.885, 0.91, 1.05, 1.11"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));

    fitpeaks.setProperty("OutputWorkspace", peak_pos_ws_name);
    fitpeaks.setProperty("OutputPeakParametersWorkspace", param_ws_name);
    fitpeaks.setProperty("FittedPeaksWorkspace", model_ws_name);

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    // Verify the existence of output workspaces
    bool peak_pos_ws_exist, peak_param_ws_exist, fitted_ws_exist;
    API::MatrixWorkspace_sptr peak_pos_ws = CheckAndRetrieveMatrixWorkspace(peak_pos_ws_name, &peak_pos_ws_exist);
    API::MatrixWorkspace_sptr model_ws = CheckAndRetrieveMatrixWorkspace(model_ws_name, &fitted_ws_exist);
    API::ITableWorkspace_sptr peak_param_ws = CheckAndRetrieveTableWorkspace(param_ws_name, &peak_param_ws_exist);

    size_t num_histograms = max_ws_index - min_ws_index + 1;

    // Expected value: with one peak that cannot be fit
    std::vector<double> exp_positions{-4, 0.8182, 0.8916, 1.0754};

    // Verify peaks' positions
    if (peak_pos_ws_exist) {
      // workspace for peak positions from fitted value: must be a 16 x 4
      // workspace2D
      TS_ASSERT_EQUALS(peak_pos_ws->getNumberHistograms(), num_histograms);
      TS_ASSERT_EQUALS(peak_pos_ws->histogram(0).y().size(), exp_positions.size());

      for (size_t ih = 0; ih < num_histograms; ++ih) {
        // Get histogram
        HistogramData::HistogramY peak_pos_i = peak_pos_ws->histogram(ih).y();
        HistogramData::HistogramE pos_error_i = peak_pos_ws->histogram(ih).e();
        // Check value
        for (size_t ip = 0; ip < exp_positions.size(); ++ip) {
          // Check fitted peak positions with the expected positions
          TS_ASSERT_DELTA(peak_pos_i[ip], exp_positions[ip], 0.0012);
          if (peak_pos_i[ip] > 0) {
            // a good fit: require error less than 100
            TS_ASSERT(pos_error_i[ip] < 150.);
          } else {
            // failed fit
            TS_ASSERT(pos_error_i[ip] > 1E20);
          }
        }
      }
    }

    // Verify calcualated model
    if (fitted_ws_exist) {
      // workspace for calculated peaks from fitted data
      TS_ASSERT_EQUALS(model_ws->getNumberHistograms(), input_ws->getNumberHistograms());
    }

    if (peak_param_ws_exist) {
      // workspace for calcualted peak parameters
      TS_ASSERT_EQUALS(peak_param_ws->rowCount(), (exp_positions.size() * num_histograms));
    }

    // clean
    AnalysisDataService::Instance().remove(input_ws_name);
    AnalysisDataService::Instance().remove(peak_pos_ws_name);
    AnalysisDataService::Instance().remove(param_ws_name);
    AnalysisDataService::Instance().remove(model_ws_name);

    FrameworkManager::Instance().setNumOMPThreadsToConfigValue();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test the optional output for fit error of each peak parameters
   * It is modified from test_multiple_peak_profiles
   * @brief test_outputFitError
   */
  void test_outputFitError() {
    // set up parameters with starting value
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createGuassParameters(peakparnames, peakparvalues);

    // Generate input workspace
    genearteTestDataGaussian(m_inputWorkspaceName);

    // initialize algorithm to test
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 2));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "5.0, 10.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowBoundaryList", "2.5, 6.5, 8.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterNames", peakparnames));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("ConstrainPeakPositions", true));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS");

    fitpeaks.setProperty("RawPeakParameters", true);
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setPropertyValue("OutputParameterFitErrorsWorkspace", "FitErrorsWS"));

    fitpeaks.execute();

    // check result
    TS_ASSERT(fitpeaks.isExecuted());
    if (!fitpeaks.isExecuted())
      return;

    // get fitted peak data
    API::MatrixWorkspace_sptr main_out_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PeakPositionsWS"));
    TS_ASSERT(main_out_ws);
    TS_ASSERT_EQUALS(main_out_ws->getNumberHistograms(), 3);

    API::MatrixWorkspace_sptr plot_ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("FittedPeaksWS"));
    TS_ASSERT(plot_ws);
    TS_ASSERT_EQUALS(plot_ws->getNumberHistograms(), 3);

    API::ITableWorkspace_sptr param_ws =
        std::dynamic_pointer_cast<API::ITableWorkspace>(AnalysisDataService::Instance().retrieve("PeakParametersWS"));
    TS_ASSERT(param_ws);
    TS_ASSERT_EQUALS(param_ws->rowCount(), 6);
    API::ITableWorkspace_sptr error_table =
        std::dynamic_pointer_cast<API::ITableWorkspace>(AnalysisDataService::Instance().retrieve("FitErrorsWS"));
    TS_ASSERT(error_table);
    // shall be same number of rows to OutputPeakParametersWorkspace
    // (PeakParametersWS)
    TS_ASSERT_EQUALS(error_table->rowCount(), param_ws->rowCount());
    // there is no Chi2 column in error table
    TS_ASSERT_EQUALS(error_table->columnCount(), param_ws->columnCount() - 1);

    // check fit error
    for (size_t irow = 0; irow < param_ws->rowCount(); ++irow) {
      continue;
    }

    // clean up
    AnalysisDataService::Instance().remove(m_inputWorkspaceName);
    AnalysisDataService::Instance().remove("PeakPositionsWS");
    AnalysisDataService::Instance().remove("FittedPeaksWS");
    AnalysisDataService::Instance().remove("PeakParametersWS");
    AnalysisDataService::Instance().remove("FitErrorsWS");
  }

  //--------------------------------------------------------------------------------------------------------------
  /** generate a peak center workspace compatible to the workspace created by
   * genearteTestDataGaussian(), which will 3 spectra and at most 2 elements for
   * each sepctrum
   * @brief genPeakCenterWorkspace
   * @param peak_index_vec :: a vector of integer as 0 or 1.  0 for peak center
   * @param workspace_name
   * @return
   */
  std::string genPeakCenterWorkspace(const std::vector<int> &peak_index_vec, const std::string &workspace_name) {
    // create the empty workspace containing N X values for N peaks (N <=2)
    size_t num_peaks = peak_index_vec.size();
    int64_t nbins = num_peaks;
    // fixed to 2 spectrum
    int64_t nhist = 3;
    // point data
    bool ishist = false;
    double xval(0), yval(0), eval(0), dxval(1);
    std::set<int64_t> maskedws;
    MatrixWorkspace_sptr center_ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(
            nhist, nbins, ishist, xval, yval, eval, dxval, maskedws));

    for (size_t i = 0; i < center_ws->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < peak_index_vec.size(); ++j) {
        const int peak_index = peak_index_vec[j];
        const double peak_center = peak_index == 0 ? 5.0 : 10.0;
        center_ws->dataX(i)[j] = peak_center;
      }
    }

    AnalysisDataService::Instance().addOrReplace(workspace_name, center_ws);

    return workspace_name;
  }

  //--------------------------------------------------------------------------------------------------------------
  /** create a fit window workspace compatibel to the workspace created by
   * genearteTestDataGaussian()
   * @brief genFitWindowWorkspace :: generate a matrix work for peak fitting
   * window
   * @param peak_index_vec :: vector for peak indexes
   * @param workspace_name :: name of the output workspace registered to ADS
   */
  std::string genFitWindowWorkspace(std::vector<int> &peak_index_vec, const std::string &workspace_name) {
    // create the empty workspace containing 3 spectrum
    const size_t num_peaks = peak_index_vec.size();
    MatrixWorkspace_sptr center_ws = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspace(3, static_cast<int>(num_peaks) * 2));
    for (size_t i = 0; i < center_ws->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < peak_index_vec.size(); ++j) {
        const int peak_index = peak_index_vec[j];
        const double peak_center = peak_index == 0 ? 5.0 : 10.0;
        center_ws->dataX(i)[j * 2] = peak_center - 2.0;
        center_ws->dataX(i)[j * 2 + 1] = peak_center + 2.0;
      }
    }

    AnalysisDataService::Instance().addOrReplace(workspace_name, center_ws);

    return workspace_name;
  }

  //--------------------------------------------------------------------------------------------------------------
  /** create basic testing data set having 3 spectra, each containing 2 Gaussian
   * peaks
   * The exact location of the peaks are at
   * ws-index = 0: peak 0 @ 5.00; peak 1  @ 10.00
   * ws-index = 1: peak 0 @ 5.01; peak 1  @  9.98
   * ws-index = 2: peak 0 @ 5.03; peak 1  @ 10.02
   * @brief genearteTestDataGaussian
   * @param workspacename
   */
  void genearteTestDataGaussian(const std::string &workspacename) {
    // ---- Create the simple workspace -------
    size_t num_spec = 3;

    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(static_cast<int>(num_spec), 300);
    WS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    // change the resolution of the binning
    for (size_t i = 0; i < num_spec; ++i)
      WS->mutableX(i) *= 0.05;

    // spectrum 1 (ws=0)
    const auto &xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(), [](const double x) {
      return exp(-0.5 * pow((x - 10) / 0.1, 2)) + 2.0 * exp(-0.5 * pow((x - 5) / 0.15, 2)) + 1;
    });
    const auto &yvals = WS->histogram(0).y();
    std::transform(yvals.cbegin(), yvals.cend(), WS->mutableE(0).begin(), [](const double y) { return 0.2 * sqrt(y); });

    if (num_spec > 1) {
      const auto &xvals1 = WS->points(1);
      std::transform(xvals1.cbegin(), xvals1.cend(), WS->mutableY(1).begin(), [](const double x) {
        return 2. * exp(-0.5 * pow((x - 9.98) / 0.12, 2)) + 4.0 * exp(-0.5 * pow((x - 5.01) / 0.17, 2)) + 1;
      });
      const auto &yvals1 = WS->histogram(1).y();
      std::transform(yvals1.cbegin(), yvals1.cend(), WS->mutableE(1).begin(),
                     [](const double y) { return 0.2 * sqrt(y); });
    }

    if (num_spec > 2) {
      const auto &xvals2 = WS->points(2);
      std::transform(xvals2.cbegin(), xvals2.cend(), WS->mutableY(2).begin(), [](const double x) {
        return 10 * exp(-0.5 * pow((x - 10.02) / 0.14, 2)) + 3.0 * exp(-0.5 * pow((x - 5.03) / 0.19, 2)) + 1;
      });
      const auto &yvals2 = WS->histogram(2).y();
      std::transform(yvals2.cbegin(), yvals2.cend(), WS->mutableE(2).begin(),
                     [](const double y) { return 0.2 * sqrt(y); });
    }

    AnalysisDataService::Instance().addOrReplace(workspacename, WS);
    return;
  }

  void createGuassParameters(vector<string> &parnames, vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("Height");
    parvalues.emplace_back(2.5e+06);

    parnames.emplace_back("Sigma");
    parvalues.emplace_back(0.1);

    parnames.emplace_back("PeakCentre");
    parvalues.emplace_back(10.0);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains peaks with profile as back to back
   * exponenential convoluted
   * by Gaussian
   */
  std::string generateTestDataBackToBackExponential() {
    DataHandling::LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename", "vulcan_diamond.nxs");
    loader.setProperty("OutputWorkspace", "diamond_3peaks");

    loader.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("diamond_3peaks"));

    API::MatrixWorkspace_sptr ws =
        std::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("diamond_3peaks"));
    TS_ASSERT(ws);

    return "diamond_3peaks";
  }

  //----------------------------------------------------------------------------------------------
  /** Generate peak parameters for Back-to-back exponential convoluted by
   * Gaussian
   * FitPeak(InputWorkspace='diamond_high_res_d', OutputWorkspace='peak0_19999',
   * ParameterTableWorkspace='peak0_19999_Param', WorkspaceIndex=19999,
   * PeakFunctionType='BackToBackExponential', PeakParameterNames='I,A,B,X0,S',
   * PeakParameterValues='2.5e+06,5400,1700,1.07,0.000355',
   * FittedPeakParameterValues='129.407,-1.82258e+06,-230935,1.06065,-0.0154214',
   * BackgroundParameterNames='A0,A1', BackgroundParameterValues='0,0',
   * FittedBackgroundParameterValues='3694.92,-3237.13', FitWindow='1.05,1.14',
   * PeakRange='1.06,1.09',
   * MinGuessedPeakWidth=10, MaxGuessedPeakWidth=20, GuessedPeakWidthStep=1,
   * PeakPositionTolerance=0.02)
   */
  void createBackToBackExponentialParameters(vector<string> &parnames, vector<double> &parvalues,
                                             bool include_pos_intensity = true) {
    parnames.clear();
    parvalues.clear();

    if (include_pos_intensity) {
      parnames.emplace_back("I");
      parvalues.emplace_back(2.5e+06);
    }

    parnames.emplace_back("A");
    parvalues.emplace_back(5400);

    parnames.emplace_back("B");
    parvalues.emplace_back(1700);

    if (include_pos_intensity) {
      parnames.emplace_back("X0");
      parvalues.emplace_back(1.07);
    }

    parnames.emplace_back("S");
    parvalues.emplace_back(0.000355);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Check whether a workspace exists or not
   * @brief CheckAndRetrieveMatrixWorkspace
   * @param ws_name
   * @param correct
   * @return
   */
  API::MatrixWorkspace_sptr CheckAndRetrieveMatrixWorkspace(const std::string &ws_name, bool *correct) {
    // retrieve workspace
    API::MatrixWorkspace_sptr workspace;
    bool exist = AnalysisDataService::Instance().doesExist(ws_name);
    TS_ASSERT(exist);
    if (!exist) {
      std::cout << "Workspace " << ws_name << " does not exist in ADS."
                << "\n";
      *correct = false;
      return workspace;
    }

    // check workspace type
    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws_name));
    TS_ASSERT(workspace);
    if (!workspace) {
      std::cout << "Workspace " << ws_name << " is not a MatrixWorkspace."
                << "\n";
      *correct = false;
      return workspace;
    }

    *correct = true;
    return workspace;
  }

  //----------------------------------------------------------------------------------------------
  /** Check whether a workspace exists or not
   * @brief CheckAndRetrieveMatrixWorkspace
   * @param ws_name
   * @param correct
   * @return
   */
  API::ITableWorkspace_sptr CheckAndRetrieveTableWorkspace(const std::string &ws_name, bool *correct) {
    // retrieve workspace
    API::ITableWorkspace_sptr workspace;
    bool exist = AnalysisDataService::Instance().doesExist(ws_name);
    TS_ASSERT(exist);
    if (!exist) {
      std::cout << "Workspace " << ws_name << " does not exist in ADS."
                << "\n";
      *correct = false;
      return workspace;
    }

    // check workspace type
    workspace = std::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve(ws_name));
    TS_ASSERT(workspace);
    if (!workspace) {
      std::cout << "Workspace " << ws_name << " is not a TableWorkspace."
                << "\n";
      *correct = false;
      return workspace;
    }

    *correct = true;
    return workspace;
  }
};
