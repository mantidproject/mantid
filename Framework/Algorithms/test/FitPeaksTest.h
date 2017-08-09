#ifndef MANTID_ALGORITHMS_FITPEAKSTEST_H_
#define MANTID_ALGORITHMS_FITPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

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
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeaksTest *createSuite() {
    API::FrameworkManager::Instance();
    return new FitPeaksTest();
  }
  static void destroySuite(FitPeaksTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test on init and setup
    */
  void test_Init() {
    // Generate input workspace
    std::string input_ws_name = loadVulcanHighAngleData();


    // Generate peak and background parameters
    std::vector<string> peakparnames;
    std::vector<double> peakparvalues;
    gen_PeakParameters(peakparnames, peakparvalues);

    // Initialize FitPeak
    FitPeaks fitpeaks;

    fitpeaks.initialize();
    TS_ASSERT(fitpeaks.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("InputWorkspace", input_ws_name));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StartWorkspaceIndex", 19999));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("StopWorkspaceIndex", 20000));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakCenters", "1.0758"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowLeftBoundary", "1.05"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("FitWindowRightBoundary", "1.15"));
    TS_ASSERT_THROWS_NOTHING(fitpeaks.setProperty("PeakParameterValues", peakparvalues));

    fitpeaks.setProperty("OutputWorkspace", "PeakPositionsWS");
    fitpeaks.setProperty("OutputPeakParametersWorkspace", "PeakParametersWS");

    fitpeaks.execute();

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
   * FittedBackgroundParameterValues='3694.92,-3237.13', FitWindow='1.05,1.14', PeakRange='1.06,1.09',
   * MinGuessedPeakWidth=10, MaxGuessedPeakWidth=20, GuessedPeakWidthStep=1, PeakPositionTolerance=0.02)
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

    API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
                AnalysisDataService::Instance().retrieve("Diamond2Peaks"));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 24900);

    return "Diamond2Peaks";
  }

};

#endif /* MANTID_ALGORITHMS_FITPEAKSTEST_H_ */
