// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <fstream>

using Mantid::Algorithms::FindPeaks;

using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Points;

namespace {
/// Load focussed data file
void loadNexusProcessed(const std::string &filename, const std::string &wsname) {
  Mantid::DataHandling::LoadNexusProcessed loader;
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setProperty("OutputWorkspace", wsname);
  loader.execute();
  TS_ASSERT(loader.isExecuted());
}

const std::vector<double> VANADIUM_CENTRES{0.5044, 0.5191, 0.5350, 0.5526, 0.5936, 0.6178, 0.6453, 0.6768, 0.7134,
                                           0.7566, 0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401};

// columns in the effective peak table
const int COL_SPECTRUM{0};
const int COL_CENTRE{1};
const int COL_WIDTH{2};
const int COL_HEIGHT{3};
const int COL_A0{4};
const int COL_A1{5};
const int COL_A2{6};
const int COL_CHISQ{7};

} // anonymous namespace

class FindPeaksTest : public CxxTest::TestSuite {
public:
  static FindPeaksTest *createSuite() { return new FindPeaksTest(); }
  static void destroySuite(FindPeaksTest *suite) { delete suite; }

  FindPeaksTest() { FrameworkManager::Instance(); }

  /// Test basic functions
  void test_TheBasics() {
    FindPeaks finder;
    TS_ASSERT_EQUALS(finder.name(), "FindPeaks");
    TS_ASSERT_EQUALS(finder.version(), 1);
  }

  /// Test initialization
  void test_Init() {
    FindPeaks finder;
    TS_ASSERT_THROWS_NOTHING(finder.initialize());
    TS_ASSERT(finder.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** Test find a single peak with given position
   */
  void test_findSinglePeakGivenPeakPosition() {
    FrameworkManager::Instance();

    MatrixWorkspace_sptr dataws = getSinglePeakData();
    std::string wsname("SinglePeakTestData");
    AnalysisDataService::Instance().addOrReplace(wsname, dataws);

    FindPeaks finder1;
    finder1.initialize();
    TS_ASSERT(finder1.isInitialized());

    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("WorkspaceIndex", "0"));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("Tolerance", 4));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("FWHM", 8));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("PeakPositions", "1.2356"));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("FitWindows", "1.21, 1.50"));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("BackgroundType", "Quadratic"));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("HighBackground", true));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("MinGuessedPeakWidth", 2));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("MaxGuessedPeakWidth", 10));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("PeakPositionTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("RawPeakParameters", true));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("CostFunction", "Chi-Square"));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("Minimizer", "Levenberg-MarquardtMD"));
    TS_ASSERT_THROWS_NOTHING(finder1.setPropertyValue("PeaksList", "FoundedSinglePeakTable"));
    TS_ASSERT_THROWS_NOTHING(finder1.setProperty("StartFromObservedPeakCentre", false));

    TS_ASSERT_THROWS_NOTHING(finder1.execute());

    TS_ASSERT(finder1.isExecuted());

    // Get output workspace
    TableWorkspace_sptr outtablews =
        std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve("FoundedSinglePeakTable"));
    TS_ASSERT(outtablews);

    // Size of the output workspace
    TS_ASSERT_EQUALS(outtablews->rowCount(), 1);
    if (outtablews->rowCount() == 0)
      return;

    map<string, double> parammap;
    getParameterMap(outtablews, 0, parammap);

    TS_ASSERT_DELTA(parammap["PeakCentre"], 1.2356, 0.03);
    TS_ASSERT_DELTA(parammap["Height"], 595., 15.00);

    // Clean
    AnalysisDataService::Instance().remove(wsname);
    AnalysisDataService::Instance().remove("FoundedSinglePeakTable");
  }

  //----------------------------------------------------------------------------------------------
  /** Test find peaks automaticallyclear
   */
  void test_findMultiPeaksAuto() {
    const std::string WKSP_NAME_INPUT("FindPeaksTest_peaksWS");
    const std::string WKSP_NAME_OUTPUT("FindPeaksTest_foundpeaks");

    // Load data file
    loadNexusProcessed("focussed.nxs", WKSP_NAME_INPUT);

    // Find peaks (Test)
    FindPeaks finder;
    if (!finder.isInitialized())
      finder.initialize();

    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("InputWorkspace", WKSP_NAME_INPUT));
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("WorkspaceIndex", "4"));
    // TS_ASSERT_THROWS_NOTHING(
    // finder.setPropertyValue("SmoothedData","smoothed") );
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("PeaksList", WKSP_NAME_OUTPUT));

    TS_ASSERT_THROWS_NOTHING(finder.execute());
    TS_ASSERT(finder.isExecuted());

    Mantid::API::ITableWorkspace_sptr peaklist = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(WKSP_NAME_OUTPUT));

    TS_ASSERT(peaklist);
    TS_ASSERT_EQUALS(peaklist->rowCount(), 9);
    // check peak positions
    TS_ASSERT_DELTA(peaklist->Double(1, COL_CENTRE), 0.59, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(2, COL_CENTRE), 0.71, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(3, COL_CENTRE), 0.81, 0.01);
    // This is a dodgy value, that comes out different on different platforms
    // TS_ASSERT_DELTA( peaklist->Double(3,COL_CENTRE), 1.03, 0.01 );
    TS_ASSERT_DELTA(peaklist->Double(5, COL_CENTRE), 0.96, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(6, COL_CENTRE), 1.24, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(7, COL_CENTRE), 1.52, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(8, COL_CENTRE), 2.14, 0.01);

    // cleanup
    AnalysisDataService::Instance().remove(WKSP_NAME_INPUT);
    AnalysisDataService::Instance().remove(WKSP_NAME_OUTPUT);
  }

  void testFindMultiPeaksGivenPeaksList() {
    const std::string WKSP_NAME_INPUT("FindPeaksTest_vanadium");
    const std::string WKSP_NAME_OUTPUT("FindPeaksTest_foundpeaks2");

    loadNexusProcessed("PG3_733_focussed.nxs", WKSP_NAME_INPUT);

    FindPeaks finder;
    if (!finder.isInitialized())
      finder.initialize();
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("InputWorkspace", WKSP_NAME_INPUT));
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("WorkspaceIndex", 0)); // only fit first spectrum
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("PeakPositions", VANADIUM_CENTRES));
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("PeaksList", WKSP_NAME_OUTPUT));

    TS_ASSERT_THROWS_NOTHING(finder.execute());
    TS_ASSERT(finder.isExecuted());

    Mantid::API::ITableWorkspace_sptr peaklist = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(WKSP_NAME_OUTPUT));
    // TODO validate the results

    // cleanup
    AnalysisDataService::Instance().remove(WKSP_NAME_INPUT);
    AnalysisDataService::Instance().remove(WKSP_NAME_OUTPUT);
  }

  // this test is meant to mimic part of the innards the SNSPowderRedux.PG3StripPeaks system test
  // StripVanadiumPeaks calls StripPeaks (with specific positions) which calls FindPeaks
  void testPG3Vanadium() {
    const std::string WKSP_NAME_INPUT("FindPeaksTest_PG3_4866");
    const std::string WKSP_NAME_OUTPUT("FindPeaksTest_PG3_4866_table");

    // copied from running the tests on ubuntu 20.04
    const std::vector<double> HEIGHTS{657., 739., 435., 636., 1167., 554., 627., 244., 829.,
                                      310., 773., 260., 306., 182.,  586., 292., 31.};
    const std::vector<double> WIDTHS{0.0007, 0.0009, 0.0001, 0.0012, 0.0013, 0.0012, 0.0013, 0.0011, 0.0015,
                                     0.0030, 0.0023, 0.0038, 0.0021, 0.0034, 0.0042, 0.0062, 0.0099};
    // peaks that have different values on differnt operating systems
    const std::vector<int> BAD{1, 3, 4, 5, 6, 9, 10, 11, 12, 13, 15, 16};

    loadNexusProcessed("PG3_4866_focussed_vanadium.nxs", WKSP_NAME_INPUT);

    FindPeaks finder;
    if (!finder.isInitialized())
      finder.initialize();
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("InputWorkspace", WKSP_NAME_INPUT));
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("PeaksList", WKSP_NAME_OUTPUT));
    // values set from system test
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("PeakPositionTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("FWHM", 8));
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("BackgroundType", "Quadratic"));
    // default values in StripVandiumPeaks/StripPeaks
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("PeakPositions", VANADIUM_CENTRES));
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("Tolerance", 4));
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("HighBackground", true));
    // get the effective even though StripPeaks sets it to true
    TS_ASSERT_THROWS_NOTHING(finder.setProperty("RawPeakParameters", false));

    // run the code
    TS_ASSERT_THROWS_NOTHING(finder.execute());
    TS_ASSERT(finder.isExecuted());

    // validate the results
    Mantid::API::ITableWorkspace_sptr peaklist = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(WKSP_NAME_OUTPUT));
    TS_ASSERT(peaklist);
    const auto NUM_PEAKS = static_cast<int>(VANADIUM_CENTRES.size());
    TS_ASSERT_EQUALS(peaklist->rowCount(), VANADIUM_CENTRES.size()); // everything requested should be present
    // TODO
    for (int i = 0; i < NUM_PEAKS; ++i) {
      // only one spectrum supplied
      TS_ASSERT_EQUALS(peaklist->Int(i, COL_SPECTRUM), 0);
      std::cout << i << " center=" << peaklist->Double(i, COL_CENTRE) << " width=" << peaklist->Double(i, COL_WIDTH)
                << " height=" << peaklist->Double(i, COL_HEIGHT) << " chisq=" << peaklist->Double(i, COL_CHISQ)
                << std::endl;
      // positions guessed were "close"
      TS_ASSERT_DELTA(peaklist->Double(i, COL_CENTRE), VANADIUM_CENTRES[i], 0.01);
      TS_ASSERT(peaklist->Double(i, COL_CHISQ) > 0.);
      // other peak shape parameters show of instability
      double tol_height = 4.;
      double tol_width = 0.0001;
      if (std::find(BAD.begin(), BAD.end(), i) != BAD.end()) {
        // "light" checks have larger tolerances
        tol_height = 10.;
        tol_width = 0.001;
      }
      // super-special peaks
      if (i == 0) {
        tol_height = 30.;
      } else if (i == 3) {
        tol_height = 30.;
      } else if (i == 4) {
        tol_height = 19.;
      } else if (i == 5) {
        tol_height = 55.;
        tol_width = 0.003;
      } else if (i == 8) {
        tol_height = 17.;
      } else if (i == 9) {
        tol_height = 36.;
        tol_width = 0.0014;
      } else if (i == 10) {
        tol_height = 11.;
      } else if (i == 13) {
        tol_height = 18.;
      } else if (i == 15) {
        tol_height = 15.;
      } else if (i == 16) {
        tol_width = 0.015;
        tol_height = 15.;
      }
      TS_ASSERT_DELTA(peaklist->Double(i, COL_HEIGHT), HEIGHTS[i], tol_height);
      TS_ASSERT_DELTA(peaklist->Double(i, COL_WIDTH), WIDTHS[i], tol_width);
    }

    // cleanup
    AnalysisDataService::Instance().remove(WKSP_NAME_INPUT);
    AnalysisDataService::Instance().remove(WKSP_NAME_OUTPUT);
  }

  void testWhenSingleBinWs() {
    MatrixWorkspace_sptr dataws = WorkspaceCreationHelper::create2DWorkspace(10, 1);
    std::string wsName("SingleBinTestData");
    AnalysisDataService::Instance().addOrReplace(wsName, dataws);
    FindPeaks finder;
    finder.setRethrows(true);

    if (!finder.isInitialized())
      finder.initialize();
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("PeaksList", "peakslist"));
    TSM_ASSERT_THROWS("Block size is not sufficient to run FindSXPeaks", finder.execute(), std::runtime_error &);
  }

  //----------------------------------------------------------------------------------------------
  /** Parse a row in output parameter tableworkspace to a string/double
   * parameter name/value map
   */
  void getParameterMap(const TableWorkspace_sptr &tablews, size_t rowindex, map<string, double> &parammap) {
    parammap.clear();

    vector<string> vecnames = tablews->getColumnNames();
    /*
    for (size_t i = 0; i < vecnames.size(); ++i)
      cout << "Column " << i << " : " << vecnames[i] << "\n";
    size_t numrows = tablews->rowCount();
    cout << "Number of rows = " << numrows << "\n";
    */

    for (size_t i = 0; i < vecnames.size(); ++i) {
      string parname = vecnames[i];
      if (parname != "spectrum") {
        double parvalue = tablews->cell<double>(rowindex, i);
        parammap.emplace(parname, parvalue);
        cout << "Add parameter " << parname << " = " << parvalue << "\n";
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a workspace as a partial data from PG3_4866 around Vanadium peak at
   * d = 1.235
   */
  MatrixWorkspace_sptr getSinglePeakData() {

    const size_t size = 83;

    MatrixWorkspace_sptr dataws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

    dataws->setHistogram(
        0, Points{1.210120, 1.210600, 1.211080, 1.211570, 1.212050, 1.212540, 1.213020, 1.213510, 1.213990, 1.214480,
                  1.214970, 1.215450, 1.215940, 1.216420, 1.216910, 1.217400, 1.217880, 1.218370, 1.218860, 1.219350,
                  1.219830, 1.220320, 1.220810, 1.221300, 1.221790, 1.222280, 1.222760, 1.223250, 1.223740, 1.224230,
                  1.224720, 1.225210, 1.225700, 1.226190, 1.226680, 1.227170, 1.227660, 1.228160, 1.228650, 1.229140,
                  1.229630, 1.230120, 1.230610, 1.231110, 1.231600, 1.232090, 1.232580, 1.233080, 1.233570, 1.234060,
                  1.234560, 1.235050, 1.235540, 1.236040, 1.236530, 1.237030, 1.237520, 1.238020, 1.238510, 1.239010,
                  1.239500, 1.240000, 1.240500, 1.240990, 1.241490, 1.241990, 1.242480, 1.242980, 1.243480, 1.243970,
                  1.244470, 1.244970, 1.245470, 1.245970, 1.246460, 1.246960, 1.247460, 1.247960, 1.248460, 1.248960,
                  1.249460, 1.249960, 1.250460},
        Counts{1619.0, 1644.0, 1616.0, 1589.0, 1608.0, 1612.0, 1630.0, 1671.0, 1588.0, 1577.0, 1616.0, 1556.0,
               1625.0, 1655.0, 1552.0, 1539.0, 1538.0, 1542.0, 1558.0, 1628.0, 1557.0, 1606.0, 1563.0, 1611.0,
               1584.0, 1447.0, 1532.0, 1580.0, 1539.0, 1513.0, 1601.0, 1558.0, 1567.0, 1573.0, 1551.0, 1465.0,
               1602.0, 1543.0, 1538.0, 1515.0, 1556.0, 1574.0, 1519.0, 1452.0, 1568.0, 1522.0, 1518.0, 1603.0,
               1538.0, 1659.0, 1685.0, 1763.0, 1846.0, 1872.0, 2018.0, 2035.0, 2113.0, 2131.0, 1921.0, 1947.0,
               1756.0, 1603.0, 1602.0, 1552.0, 1558.0, 1518.0, 1512.0, 1511.0, 1466.0, 1474.0, 1368.0, 1463.0,
               1447.0, 1409.0, 1381.0, 1478.0, 1445.0, 1429.0, 1447.0, 1354.0, 1430.0, 1440.0, 1423.0});

    return dataws;
  }

private:
};

//=================================================================================================
/** Performance test with large workspaces.
 */

class FindPeaksTestPerformance : public CxxTest::TestSuite {
public:
  static FindPeaksTestPerformance *createSuite() { return new FindPeaksTestPerformance(); }
  static void destroySuite(FindPeaksTestPerformance *suite) { delete suite; }

  /** Constructor
   */
  FindPeaksTestPerformance() {}

  /** Set up workspaces
   */
  void setUp() override {
    // Load data file
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", "focussed.nxs");
    loader.setProperty("OutputWorkspace", "FindPeaksTest_peaksWS");
    loader.execute();

    m_dataWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("FindPeaksTest_peaksWS"));

    m_syntheticWS = createSyntheticWS();

    return;
  }

  /** Find peaks by auto-determine peaks' positions
   */
  void test_FindPeaksAutoPeakPositions() {
    // Find peaks (Test)
    FindPeaks finder;
    if (!finder.isInitialized())
      finder.initialize();

    // The data file has 5 spectra, each of them with up to 5-6 peaks
    if (!m_dataWS)
      throw std::runtime_error("Unable to get input matrix workspace. ");
    finder.setPropertyValue("InputWorkspace", "FindPeaksTest_peaksWS");
    finder.setPropertyValue("PeakPositions", "0.8089, 0.9571, 1.0701,1.2356,1.5133,2.1401");
    finder.setPropertyValue("PeaksList", "FindPeaksTest_foundpeaks");

    finder.execute();
  }

  /*
   * Gives FindPeaks a synthetic spectrum with 10 peaks of various
   * shapes, without 'PeakPositions' hint.
   */
  void test_singleSpectrumMultiplePeaksNoPeaksPositions() {
    FindPeaks fp;
    fp.setChild(true);
    fp.initialize();
    fp.setChild(true);
    fp.setProperty("InputWorkspace", m_syntheticWS);
    fp.setPropertyValue("PeaksList", "FindPeaksTest_foundpeaks");
    fp.execute();
    auto tbl = fp.getProperty("PeaksList");
  }

private:
  /**
   * Creates a synthetic single-spectrum workspace with synthetic data
   * containing some peaks.
   */
  Mantid::API::MatrixWorkspace_sptr createSyntheticWS() const {
    Mantid::Algorithms::CreateSampleWorkspace create;
    create.initialize();
    create.setChild(true);
    create.setPropertyValue("Function", "User Defined");
    create.setPropertyValue("UserDefinedFunction", makeSpectrum10Peaks());
    create.setProperty("NumBanks", 1);
    create.setProperty("BankPixelWidth", 1);
    create.setProperty("XMin", 0.0);
    create.setProperty("XMax", 200.0);
    create.setProperty("BinWidth", 0.1);
    create.setPropertyValue("OutputWorkspace", "FindPeaksTestPerf_peaks_ws");
    create.execute();
    return create.getProperty("OutputWorkspace");
  }

  /**
   * Produces a user defined function with 10 peaks to keep FindPeaks
   * busy for a small fraction of a second.  Some peaks are not easy
   * (simulated here with different shapes).
   */
  std::string makeSpectrum10Peaks() const {
    const std::string def = "name=LinearBackground, A0=101.3, A1=8510.3;"
                            "name=Gaussian, PeakCentre=0.1, Height=200000, Sigma=0.007;"
                            "name=Lorentzian, PeakCentre=0.45, Amplitude=6000, FWHM=0.017;"
                            "name=PseudoVoigt, PeakCentre=0.75, Intensity=85050, FWHM=0.04;"
                            "name=Gaussian, PeakCentre=0.95, Height=110000, Sigma=0.007;"
                            "name=Gaussian, PeakCentre=1.15, Height=110000, Sigma=0.007;"
                            "name=BackToBackExponential, X0=1.30, I=7000, A=1500.1, B=1800.2, "
                            "S=0.01;"
                            "name=Gaussian, PeakCentre=1.50, Height=29000, Sigma=0.01;"
                            "name=Gaussian, PeakCentre=1.70, Height=90000, Sigma=0.02;"
                            "name=Gaussian, PeakCentre=1.90, Height=80000, Sigma=0.007;"
                            "name=Gaussian, PeakCentre=2.1, Height=150000, Sigma=0.007;";
    return def;
  }

  Mantid::API::MatrixWorkspace_sptr m_dataWS;
  Mantid::API::MatrixWorkspace_sptr m_syntheticWS;
}; // end of class FindPeaksTestPerformance
