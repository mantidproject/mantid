#ifndef MANTID_ALGORITHMS_FITPEAKSTEST_H_
#define MANTID_ALGORITHMS_FITPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
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

using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;

class FitPeaksTest : public CxxTest::TestSuite {
private:
  std::string m_inputWorkspaceName;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeaksTest *createSuite() {
    API::FrameworkManager::Instance();
    return new FitPeaksTest();
  }
  static void destroySuite(FitPeaksTest *suite) { delete suite; }

  void test_Init() {
    // Generate input workspace
    // m_inputWorkspaceName = loadVulcanHighAngleData();
    m_inputWorkspaceName = "temp_workspace";
    createTestData(m_inputWorkspaceName);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  void test_multiPeaksMultiSpectra() {
    // set up parameters with starting value
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    createTestParameters(peakparnames, peakparvalues);

    // initialize algorithm to test
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 2));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "5.0, 10.0"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowBoundaryList", "2.5, 6.5, 8.0, 12.0"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakParameterValues", peakparvalues));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("HighBackground", false));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS");

    fitpeaks.execute();

    // check result
    TS_ASSERT(fitpeaks.isExecuted());

    // get fitted peak data
    API::MatrixWorkspace_sptr plot_ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("FittedPeaksWS"));
    TS_ASSERT(plot_ws);
    TS_ASSERT_EQUALS(plot_ws->getNumberHistograms(), 3);

    API::ITableWorkspace_sptr param_ws =
        boost::dynamic_pointer_cast<API::ITableWorkspace>(
            AnalysisDataService::Instance().retrieve("PeakParametersWS"));
    TS_ASSERT(param_ws);
    TS_ASSERT_EQUALS(param_ws->rowCount(), 6);

    // TODO ASAP : check values
  }

  //----------------------------------------------------------------------------------------------
  /** Test on single peak and 1 spectrum among many
    */
  void Ntest_singlePeakSpectrum() {
    // Generate input workspace
    // std::string input_ws_name = loadVulcanHighAngleData();

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("StartWorkspaceIndex", 19014));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 19015));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "1.0758"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowLeftBoundary", "1.05"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowRightBoundary", "1.15"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakParameterValues", peakparvalues));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS");

    fitpeaks.execute();

    //  MatrixWorkspace_const_sptr outws3 =
    //  fitpeaks.getProperety("FittedPeaksWS");
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("FittedPeaksWS"));

    // clean

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on single peak on partial spectra
    */
  void Ntest_singlePeakMultiSpectra() {
    // Generate input workspace
    // std::string input_ws_name = loadVulcanHighAngleData();

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("StartWorkspaceIndex", 19990));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 20000));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "1.0758"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowLeftBoundary", "1.05"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowRightBoundary", "1.15"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakRanges", "0.02"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakParameterValues", peakparvalues));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS3");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS3");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS3");

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    // check output workspaces
    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("PeakPositionsWS3"));
    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("PeakParametersWS3"));
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("FittedPeaksWS3"));

    // about the parameters
    API::MatrixWorkspace_sptr peak_params_ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("PeakParametersWS3"));
    TS_ASSERT(peak_params_ws);
    TS_ASSERT_EQUALS(peak_params_ws->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(peak_params_ws->histogram(0).x().size(), 10);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on init and setup
    */
  void Ntest_SingleSpectrum3Peaks() {
    // Generate input workspace
    // std::string input_ws_name = loadVulcanHighAngleData();

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 6468));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 24900));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakCenters", "1.0758, 0.89198"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowLeftBoundary", "1.05, 0.87"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("FitWindowRightBoundary", "1.15, 0.92"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakRanges", "0.02, 0.015"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakParameterValues", peakparvalues));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS2");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS2");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS2");

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("PeakPositionsWS2"));

    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("FittedPeaksWS2"));
    API::MatrixWorkspace_sptr fitted_data_ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            API::AnalysisDataService::Instance().retrieve("FittedPeaksWS2"));
    TS_ASSERT(fitted_data_ws);

    // API::MatrixWorkspace_const_sptr fitted_data_ws =
    // fitpeaks.getProperty("FittedPeaksWS2");
    TS_ASSERT_EQUALS(fitted_data_ws->getNumberHistograms(), 24900);

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
    IAlgorithm *loader =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadNexus");
    loader->setPropertyValue("Filename", "PG3_733.nxs");
    loader->setPropertyValue("OutputWorkspace", input_ws_name);
    loader->execute();
    TS_ASSERT(loader->isExecuted());

    // Initialize FitPeak
    FitPeaks fit_peaks_alg;

    fit_peaks_alg.initialize();
    TS_ASSERT(fit_peaks_alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fit_peaks_alg.setProperty("InputWorkspace", input_ws_name));

    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty(
        "PeakCenters", "0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0."
                       "6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,"
                       "1.5133,2.1401"));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("FitFromRight", true));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty("HighBackground", true));
    TS_ASSERT_THROWS_NOTHING(fit_peaks_alg.setProperty(
        "PeakWidthPercent", 0.016)); // typical powgen's

    std::string output_ws_name("PG3_733_stripped");
    std::string peak_pos_ws_name("PG3_733_peak_positions");
    std::string peak_param_ws_name("PG3_733_peak_params");
    fit_peaks_alg.setProperty("OutputWorkspace", peak_pos_ws_name);
    fit_peaks_alg.setProperty("OutputPeakParametersWorkspace",
                              peak_param_ws_name);
    fit_peaks_alg.setProperty("FittedPeaksWorkspace", output_ws_name);

    fit_peaks_alg.execute();
    TS_ASSERT(fit_peaks_alg.isExecuted());

    // Clean up
    AnalysisDataService::Instance().remove(input_ws_name);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test on init and setup
    */
  void Later_test_highAngle4Peaks() {
    // Generate input workspace
    // std::string input_ws_name = loadVulcanHighAngleData();

    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("InputWorkspace", m_inputWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 6468));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 24900));
    // 0.728299, 0.89198, 1.07577, 1.26145
    // 0.6307, 0.6867, 0.728299, 0.89198, 1.07577, 1.26145
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty(
        "PeakCenters", "1.0758, 0.89198, 0.728299, 0.6867"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowLeftBoundary",
                                                  "1.05, 0.87, 0.71, 0.67"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowRightBoundary",
                                                  "1.15, 0.92, 0.76, 0.709"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakRanges", "0.02, 0.015, 0.01, 0.01"));
    TS_ASSERT_THROWS_NOTHING(
        fitpeaks.setProperty("PeakParameterValues", peakparvalues));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS2");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS2");
    fitpeaks.setProperty("FittedPeaksWorkspace", "FittedPeaksWS2");

    fitpeaks.execute();
    TS_ASSERT(fitpeaks.isExecuted());

    TS_ASSERT(
        API::AnalysisDataService::Instance().doesExist("PeakPositionsWS2"));

    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("FittedPeaksWS2"));
    API::MatrixWorkspace_sptr fitted_data_ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            API::AnalysisDataService::Instance().retrieve("FittedPeaksWS2"));
    TS_ASSERT(fitted_data_ws);

    // API::MatrixWorkspace_const_sptr fitted_data_ws =
    // fitpeaks.getProperty("FittedPeaksWS2");
    TS_ASSERT_EQUALS(fitted_data_ws->getNumberHistograms(), 24900);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
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
  void gen_PeakParameters(vector<string> &parnames, vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("I");
    parvalues.push_back(2.5e+06);

    parnames.emplace_back("A");
    parvalues.push_back(5400);

    parnames.emplace_back("B");
    parvalues.push_back(1700);

    parnames.emplace_back("X0");
    parvalues.push_back(1.07);

    parnames.emplace_back("S");
    parvalues.push_back(0.000355);

    return;
  }

  void createTestData(const std::string &workspacename) {
    // ---- Create the simple workspace -------
    size_t num_spec = 3;

    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            static_cast<int>(num_spec), 300);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    // change the resolution of the binning
    for (size_t i = 0; i < num_spec; ++i)
      WS->mutableX(i) *= 0.05;

    auto xvals = WS->points(0);
    std::transform(xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
                   [](const double x) {
                     return exp(-0.5 * pow((x - 10) / 0.1, 2)) +
                            2.0 * exp(-0.5 * pow((x - 5) / 0.15, 2));
                   });

    if (num_spec > 1) {
      auto xvals1 = WS->points(1);
      std::transform(xvals1.cbegin(), xvals1.cend(), WS->mutableY(1).begin(),
                     [](const double x) {
                       return 2. * exp(-0.5 * pow((x - 9.98) / 0.12, 2)) +
                              4.0 * exp(-0.5 * pow((x - 5.01) / 0.17, 2));
                     });
    }

    if (num_spec > 2) {
      auto xvals2 = WS->points(2);
      std::transform(xvals2.cbegin(), xvals2.cend(), WS->mutableY(2).begin(),
                     [](const double x) {
                       return 10 * exp(-0.5 * pow((x - 10.02) / 0.14, 2)) +
                              3.0 * exp(-0.5 * pow((x - 5.03) / 0.19, 2));
                     });
    }

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    AnalysisDataService::Instance().addOrReplace(workspacename, WS);

    //    auto vecx = WS->x(2);
    //    auto vecy = WS->y(2);
    //    for (size_t i = 0; i < vecx.size(); ++i)
    //      std::cout << vecx[i] << "\t" << vecy[i] << "\n";

    return;
  }

  void createTestParameters(vector<string> &parnames,
                            vector<double> &parvalues) {
    parnames.clear();
    parvalues.clear();

    parnames.emplace_back("I");
    parvalues.push_back(2.5e+06);

    parnames.emplace_back("S");
    parvalues.push_back(0.1);

    parnames.emplace_back("X0");
    parvalues.push_back(10.0);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  std::string loadVulcanHighAngleData() {

    DataHandling::LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename", "/home/wzz/Mantid/VULCAN_150178_2Peaks.nxs");
    loader.setProperty("OutputWorkspace", "Diamond2Peaks");

    loader.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Diamond2Peaks"));

    API::MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Diamond2Peaks"));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 24900);

    return "Diamond2Peaks";
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  std::string loadVulcanHighAngleDataFull() {

    DataHandling::LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename", "/home/wzz/Mantid/VULCAN_150178_2Peaks.nxs");
    loader.setProperty("OutputWorkspace", "Diamond2Peaks");

    loader.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Diamond2Peaks"));

    API::MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("Diamond2Peaks"));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 24900);

    return "Diamond2Peaks";
  }
};

#endif /* MANTID_ALGORITHMS_FITPEAKSTEST_H_ */
