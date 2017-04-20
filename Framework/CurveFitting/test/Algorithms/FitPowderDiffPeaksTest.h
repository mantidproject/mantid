#ifndef MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_
#define MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/FitPowderDiffPeaks.h"
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <fstream>

using Mantid::CurveFitting::Algorithms::FitPowderDiffPeaks;

using namespace std;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;

// To define methods shared between unit and performance tests
namespace {

//----------------------------------------------------------------------------------------------
/** Import data from a column data file by calling LoadAscii
*/
void importDataFromColumnFile(string filename, string datawsname) {
  // 1. Call LoadAscii
  DataHandling::LoadAscii2 loader;
  loader.initialize();

  loader.setProperty("FileName", filename);
  loader.setProperty("OutputWorkspace", datawsname);
  loader.setProperty("Separator", "Space");
  loader.setProperty("Unit", "TOF");

  loader.execute();

  if (!loader.isExecuted()) {
    stringstream errss;
    errss << "Failed to load file " << filename << " by calling LoadAsci()";
    throw runtime_error(errss.str());
  }

  return;
}

//----------------------------   Diffraction Data [From File]
//----------------------------------
//----------------------------------------------------------------------------------------------
/** Create data workspace
*  Option 1: Old Bank 7 data
*         2: New Bank 1 data
*/
API::MatrixWorkspace_sptr createInputDataWorkspace(int option) {
  // 1. Import data
  string datawsname("Data");

  switch (option) {
  case 1:
    importDataFromColumnFile(
        "/home/wzz/Mantid/Code/debug/MyTestData/4862b7.inp", datawsname);
    std::cout << "Option 1:  4862b7.inp. \n";
    break;

  case 2:
    importDataFromColumnFile("PG3_10808-1.dat", datawsname);
    std::cout << "Option 2:  PG3_10808-1.dat. \n";
    break;

  default:
    // not supported
    std::cout << "LeBailFitTest.createInputDataWorkspace() Option " << option
              << " is not supported. \n";
    throw std::invalid_argument("Unsupported option. ");
  }

  // 2. Get workspace
  MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(datawsname));
  if (!dataws) {
    throw runtime_error("Failed to retrieve data workspace from LoadAsii.");
  }

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Create the Bragg peak parameters table for LaB6, PG3, Bank1
*/
void createLaB6PG3Bank1BraggPeaksTable(TableWorkspace_sptr tablews) {
  TableRow newrow0 = tablews->appendRow();
  newrow0 << 6 << 3 << 1 << .6129000 << 13962.47 << 0.20687 << 0.10063
          << 62.64174 << 0.00000;
  TableRow newrow1 = tablews->appendRow();
  newrow1 << 6 << 3 << 0 << .6196725 << 14116.84 << 0.20173 << 0.09910
          << 65.37142 << 0.00000;
  TableRow newrow2 = tablews->appendRow();
  newrow2 << 6 << 2 << 2 << .6266747 << 14276.45 << 0.19651 << 0.09754
          << 68.28736 << 0.00000;
  TableRow newrow3 = tablews->appendRow();
  newrow3 << 5 << 3 << 3 << .6339198 << 14441.60 << 0.19124 << 0.09594
          << 71.40701 << 0.00000;
  TableRow newrow4 = tablews->appendRow();
  newrow4 << 5 << 4 << 1 << .6414220 << 14612.62 << 0.18590 << 0.09432
          << 74.74986 << 0.00000;
  TableRow newrow5 = tablews->appendRow();
  newrow5 << 6 << 2 << 1 << .6491972 << 14789.86 << 0.18053 << 0.09266
          << 78.33788 << 0.00000;
  TableRow newrow6 = tablews->appendRow();
  newrow6 << 6 << 2 << 0 << .6572620 << 14973.71 << 0.17512 << 0.09098
          << 82.19574 << 0.00000;
  TableRow newrow7 = tablews->appendRow();
  newrow7 << 6 << 1 << 1 << .6743366 << 15362.95 << 0.16425 << 0.08752
          << 90.83628 << 0.00000;
  TableRow newrow8 = tablews->appendRow();
  newrow8 << 6 << 1 << 0 << .6833885 << 15569.31 << 0.15882 << 0.08575
          << 95.68645 << 0.00000;
  TableRow newrow9 = tablews->appendRow();
  newrow9 << 6 << 0 << 0 << .6928150 << 15784.22 << 0.15339 << 0.08395
          << 100.94289 << 0.00000;
  TableRow newrow10 = tablews->appendRow();
  newrow10 << 5 << 3 << 1 << .7026426 << 16008.27 << 0.14798 << 0.08213
           << 106.65237 << 0.00000;
  TableRow newrow11 = tablews->appendRow();
  newrow11 << 5 << 3 << 0 << .7129008 << 16242.14 << 0.14261 << 0.08028
           << 112.86885 << 0.00000;
  TableRow newrow12 = tablews->appendRow();
  newrow12 << 5 << 2 << 2 << .7236217 << 16486.56 << 0.13728 << 0.07842
           << 119.65435 << 0.00000;
  TableRow newrow13 = tablews->appendRow();
  newrow13 << 4 << 4 << 0 << .7348413 << 16742.36 << 0.13200 << 0.07653
           << 127.08086 << 0.00000;
  TableRow newrow14 = tablews->appendRow();
  newrow14 << 5 << 2 << 1 << .7589408 << 17291.82 << 0.12165 << 0.07271
           << 144.20578 << 0.00000;
  TableRow newrow15 = tablews->appendRow();
  newrow15 << 5 << 2 << 0 << .7719151 << 17587.63 << 0.11659 << 0.07078
           << 154.11699 << 0.00000;
  TableRow newrow16 = tablews->appendRow();
  newrow16 << 5 << 1 << 1 << .7999938 << 18227.82 << 0.10675 << 0.06688
           << 177.32069 << 0.00000;
  TableRow newrow17 = tablews->appendRow();
  newrow17 << 5 << 1 << 0 << .8152332 << 18575.28 << 0.10199 << 0.06492
           << 190.96744 << 0.00000;
  TableRow newrow18 = tablews->appendRow();
  newrow18 << 5 << 0 << 0 << .8313780 << 18943.37 << 0.09733 << 0.06296
           << 206.27393 << 0.00000;
  TableRow newrow19 = tablews->appendRow();
  newrow19 << 4 << 2 << 2 << .8485216 << 19334.24 << 0.09279 << 0.06099
           << 223.52153 << 0.00000;
  TableRow newrow20 = tablews->appendRow();
  newrow20 << 3 << 3 << 2 << .8862519 << 20194.47 << 0.08407 << 0.05707
           << 265.29507 << 0.00000;
  TableRow newrow21 = tablews->appendRow();
  newrow21 << 4 << 2 << 1 << .9071078 << 20669.96 << 0.07989 << 0.05511
           << 290.77103 << 0.00000;
  TableRow newrow22 = tablews->appendRow();
  newrow22 << 4 << 2 << 0 << .9295089 << 21180.66 << 0.07585 << 0.05317
           << 320.14307 << 0.00000;
  TableRow newrow23 = tablews->appendRow();
  newrow23 << 3 << 3 << 1 << .9536560 << 21731.16 << 0.07194 << 0.05123
           << 354.25049 << 0.00000;
  TableRow newrow24 = tablews->appendRow();
  newrow24 << 4 << 1 << 1 << .9797884 << 22326.89 << 0.06815 << 0.04931
           << 394.17169 << 0.00000;
  TableRow newrow25 = tablews->appendRow();
  newrow25 << 4 << 1 << 0 << 1.008194 << 22974.43 << 0.06450 << 0.04740
           << 441.31073 << 0.00000;
  TableRow newrow26 = tablews->appendRow();
  newrow26 << 4 << 0 << 0 << 1.039222 << 23681.73 << 0.06098 << 0.04551
           << 497.52353 << 0.00000;
  TableRow newrow27 = tablews->appendRow();
  newrow27 << 3 << 2 << 1 << 1.110976 << 25317.22 << 0.05433 << 0.04178
           << 648.06329 << 0.00000;
  TableRow newrow28 = tablews->appendRow();
  newrow28 << 3 << 2 << 0 << 1.152914 << 26273.04 << 0.05119 << 0.03995
           << 750.57770 << 0.00000;
  TableRow newrow29 = tablews->appendRow();
  newrow29 << 2 << 2 << 2 << 1.199991 << 27345.91 << 0.04818 << 0.03814
           << 879.68634 << 0.00000;
  TableRow newrow30 = tablews->appendRow();
  newrow30 << 3 << 1 << 1 << 1.253349 << 28561.85 << 0.04528 << 0.03635
           << 1045.47131 << 0.00000;
  TableRow newrow31 = tablews->appendRow();
  newrow31 << 3 << 1 << 0 << 1.314524 << 29955.77 << 0.04250 << 0.03458
           << 1263.29260 << 0.00000;
  TableRow newrow32 = tablews->appendRow();
  newrow32 << 3 << 0 << 0 << 1.385630 << 31575.84 << 0.03983 << 0.03283
           << 1557.48718 << 0.00000;
  TableRow newrow33 = tablews->appendRow();
  newrow33 << 2 << 2 << 0 << 1.469683 << 33490.69 << 0.03726 << 0.03110
           << 1968.49475 << 0.00000;
  TableRow newrow34 = tablews->appendRow();
  newrow34 << 2 << 1 << 1 << 1.697043 << 38669.41 << 0.03241 << 0.02766
           << 3489.94580 << 0.00000;
  TableRow newrow35 = tablews->appendRow();
  newrow35 << 2 << 1 << 0 << 1.859018 << 42358.14 << 0.03010 << 0.02593
           << 5018.61084 << 0.00000;
  TableRow newrow36 = tablews->appendRow();
  newrow36 << 2 << 0 << 0 << 2.078445 << 47354.61 << 0.02785 << 0.02417
           << 7830.77881 << 0.00000;
  TableRow newrow37 = tablews->appendRow();
  newrow37 << 1 << 1 << 1 << 2.399981 << 54672.87 << 0.03776 << 0.18427
           << 9038.83203 << 0.00000;
  TableRow newrow38 = tablews->appendRow();
  newrow38 << 1 << 1 << 0 << 2.939365 << 68507.29 << 0.01856 << 0.01574
           << 10828.14648 << 0.00000;
  TableRow newrow39 = tablews->appendRow();
  newrow39 << 1 << 0 << 0 << 4.156890 << 89444.45 << 0.01954 << 0.01041
           << 52485.62500 << 0.00000;

  return;
}

// =============================  Reflection [From File]
// =======================================
//----------------------------------------------------------------------------------------------
/** Create reflection table workspaces
*/
DataObjects::TableWorkspace_sptr createReflectionWorkspace(int option) {
  // 1. Create table workspace
  DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
  DataObjects::TableWorkspace_sptr hklws =
      DataObjects::TableWorkspace_sptr(tablews);

  tablews->addColumn("int", "H");
  tablews->addColumn("int", "K");
  tablews->addColumn("int", "L");
  tablews->addColumn("double", "PeakHeight");
  tablews->addColumn("double", "TOF_h");
  tablews->addColumn("double", "Alpha");
  tablews->addColumn("double", "Beta");
  tablews->addColumn("double", "Sigma2");
  tablews->addColumn("double", "Gamma");

  // 2. Add reflections and heights
  switch (option) {
  case 1:
    createLaB6PG3Bank1BraggPeaksTable(hklws);
    break;

  default:
    stringstream errss;
    errss << "createReflectionWorkspace does not support option " << option
          << '\n';
    errss << "Supported options include 1 (LaB6 for PG3 bank 1). ";
    throw invalid_argument(errss.str());
  }

  // 3. Output information
  std::cout << "Created Table Workspace with " << hklws->rowCount()
            << " entries of peaks.\n";

  return hklws;
}

//----------------------------------------------------------------------------------------------
/** Add rows for input table workspace for PG3 bank1
*/
void createPG3Bank1ParameterTable(TableWorkspace_sptr tablews) {
  TableRow newrow0 = tablews->appendRow();
  newrow0 << "Alph0" << 2.708;
  TableRow newrow1 = tablews->appendRow();
  newrow1 << "Alph0t" << 79.58;
  TableRow newrow2 = tablews->appendRow();
  newrow2 << "Alph1" << 0.611;
  TableRow newrow3 = tablews->appendRow();
  newrow3 << "Alph1t" << 0.0;
  TableRow newrow4 = tablews->appendRow();
  newrow4 << "Beta0" << 2.873;
  TableRow newrow5 = tablews->appendRow();
  newrow5 << "Beta0t" << 67.52;
  TableRow newrow6 = tablews->appendRow();
  newrow6 << "Beta1" << 9.324;
  TableRow newrow7 = tablews->appendRow();
  newrow7 << "Beta1t" << 0.0;
  TableRow newrow8 = tablews->appendRow();
  newrow8 << "Dtt1" << 22583.6;
  TableRow newrow9 = tablews->appendRow();
  newrow9 << "Dtt1t" << 22334.7;
  TableRow newrow10 = tablews->appendRow();
  newrow10 << "Dtt2" << 0.0;
  TableRow newrow11 = tablews->appendRow();
  newrow11 << "Dtt2t" << 53.7626;
  TableRow newrow12 = tablews->appendRow();
  newrow12 << "Gam0" << 0.0;
  TableRow newrow13 = tablews->appendRow();
  newrow13 << "Gam1" << 0.0;
  TableRow newrow14 = tablews->appendRow();
  newrow14 << "Gam2" << 0.0;
  TableRow newrow15 = tablews->appendRow();
  newrow15 << "LatticeConstant" << 4.15689;
  TableRow newrow16 = tablews->appendRow();
  newrow16 << "Sig0" << 0.0;
  TableRow newrow17 = tablews->appendRow();
  newrow17 << "Sig1" << 10.0;
  TableRow newrow18 = tablews->appendRow();
  newrow18 << "Sig2" << 417.3;
  TableRow newrow19 = tablews->appendRow();
  newrow19 << "Tcross" << 0.356;
  TableRow newrow20 = tablews->appendRow();
  newrow20 << "Width" << 5.00256;
  TableRow newrow21 = tablews->appendRow();
  newrow21 << "Zero" << 0.0;
  TableRow newrow22 = tablews->appendRow();
  newrow22 << "Zerot" << 499.99;

  return;
}

// ============================  Instrument Parameters [From File]
// =============================
//----------------------------------------------------------------------------------------------
/** Create instrument geometry parameter/LeBail parameter workspaces
*/
DataObjects::TableWorkspace_sptr
createInstrumentParameterWorkspace(int option) {
  // 1. Crate table workspace
  DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
  DataObjects::TableWorkspace_sptr geomws =
      DataObjects::TableWorkspace_sptr(tablews);

  std::vector<std::string> paramnames{"Zero",  "Zerot",          "Dtt1",
                                      "Dtt1t", "Dtt2t",          "Tcross",
                                      "Width", "LatticeConstant"};

  tablews->addColumn("str", "Name");
  tablews->addColumn("double", "Value");

  // 2. Add peak parameters' name and values
  switch (option) {
  case 1:
    createPG3Bank1ParameterTable(geomws);
    break;

  default:
    stringstream errss;
    errss << "Option " << option
          << " is not supported by createInstrumentParameterWorkspace.\n";
    errss << "Supported options are 1 (PG3 bank1). ";
    throw invalid_argument(errss.str());
    break;
  }

  return geomws;
}
}

class FitPowderDiffPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPowderDiffPeaksTest *createSuite() {
    return new FitPowderDiffPeaksTest();
  }
  static void destroySuite(FitPowderDiffPeaksTest *suite) { delete suite; }

  /** Test init
    */
  void test_Init() {
    Algorithms::FitPowderDiffPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    return;
  }

  /** Fit the parameters for PG3's bank 1 with
    * 1. Quite-off starting values of instrumental geometry parameters.
    * 2. Quite-close starting values of peak profile parameters.
    */
  void test_RobustFitPG3Bank1() {
    // 1. Generate testing workspace
    // Data
    API::MatrixWorkspace_sptr dataws = createInputDataWorkspace(2);

    // Bragg peaks: ~/Mantid/Code/debug/MyTestData/Bank1PeaksParameters.txt
    DataObjects::TableWorkspace_sptr peakparamws = createReflectionWorkspace(1);

    // Instrument profile
    DataObjects::TableWorkspace_sptr geomparamws =
        createInstrumentParameterWorkspace(1);

    AnalysisDataService::Instance().addOrReplace("DataWorkspace", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters",
                                                 geomparamws);

    // 2. Fit
    FitPowderDiffPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "FittedPeaks");
    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputBraggPeakParameterWorkspace", "PeaksParameterTable");
    alg.setProperty("OutputZscoreWorkspace", "ZscoreTable");
    alg.setProperty("WorkspaceIndex", 0);

    alg.setProperty("MinTOF", 19650.0);
    alg.setProperty("MaxTOF", 49000.0);

    vector<int32_t> minhkl(3); // HKL = (331)
    minhkl[0] = 3;
    minhkl[1] = 3;
    minhkl[2] = 1;
    alg.setProperty("MinimumHKL", minhkl);
    alg.setProperty("NumberPeaksToFitBelowLowLimit", 2);

    alg.setProperty("FittingMode", "Robust");
    alg.setProperty("MinimumPeakHeight", 0.5);

    // Right most peak (200)
    vector<int> rightmostpeakhkl(3);
    rightmostpeakhkl[0] = 2;
    rightmostpeakhkl[1] = 0;
    rightmostpeakhkl[2] = 0;
    alg.setProperty("RightMostPeakHKL", rightmostpeakhkl);

    alg.setProperty("RightMostPeakLeftBound", 46300.0);
    alg.setProperty("RightMostPeakRightBound", 47903.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check result
    DataObjects::Workspace2D_sptr peakdataws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            AnalysisDataService::Instance().retrieve("FittedPeaks"));
    TS_ASSERT(peakdataws);
    if (!peakdataws) {
      return;
    }
    TS_ASSERT_EQUALS(peakdataws->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(peakdataws->blocksize(), 2284);
    TS_ASSERT_EQUALS(peakdataws->x(0).rawData(), peakdataws->x(1).rawData());
    TS_ASSERT_EQUALS(peakdataws->x(0).rawData(), peakdataws->x(2).rawData());
    TS_ASSERT_EQUALS(peakdataws->x(0).rawData(), peakdataws->x(3).rawData());
    TS_ASSERT_EQUALS(peakdataws->x(0).rawData(), peakdataws->x(4).rawData());
    TS_ASSERT_DELTA(peakdataws->y(0)[0], 0.4302, 0.0001);
    TS_ASSERT_DELTA(peakdataws->y(2)[0], 0.4302, 0.0001);
    TS_ASSERT_DELTA(peakdataws->y(0)[500], 0.4163, 0.0001);
    TS_ASSERT_DELTA(peakdataws->y(2)[500], 0.4163, 0.0001);
    TS_ASSERT_DELTA(peakdataws->y(0)[1000], 0.4331, 0.0001);
    TS_ASSERT_DELTA(peakdataws->y(2)[1000], 0.4331, 0.0001);

    // Output Bragg peaks parameters
    DataObjects::TableWorkspace_sptr outbraggws =
        boost::dynamic_pointer_cast<TableWorkspace>(
            AnalysisDataService::Instance().retrieve("PeaksParameterTable"));
    TS_ASSERT(outbraggws);
    if (!outbraggws) {
      return;
    }
    TS_ASSERT_EQUALS(outbraggws->rowCount(), 11);
    TS_ASSERT_EQUALS(outbraggws->columnCount(), 10);
    TS_ASSERT_DELTA(outbraggws->Double(0, 9), 1.83, 0.01);
    TS_ASSERT_DELTA(outbraggws->Double(4, 9), 0.44, 0.01);
    TS_ASSERT_DELTA(outbraggws->Double(8, 9), 0.52, 0.01);

    AnalysisDataService::Instance().remove("DataWorkspace");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("InstrumentParameters");
    AnalysisDataService::Instance().remove("FittedPeaks");
    AnalysisDataService::Instance().remove("PeaksParameterTable");
  }
};

class FitPowderDiffPeaksTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPowderDiffPeaksTestPerformance *createSuite() {
    return new FitPowderDiffPeaksTestPerformance();
  }
  static void destroySuite(FitPowderDiffPeaksTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    // Test workspace
    dataws = createInputDataWorkspace(2);
    // Bragg peaks
    peakparamws = createReflectionWorkspace(1);
    // Instrument profile
    geomparamws = createInstrumentParameterWorkspace(1);

    AnalysisDataService::Instance().addOrReplace("DataWorkspace", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters",
                                                 geomparamws);
  }

  void test_RobustFitPG3Bank1() {
    /** Fit the parameters for PG3's bank 1 with
    * 1. Quite-off starting values of instrumental geometry parameters.
    * 2. Quite-close starting values of peak profile parameters.
    */

    // HKL = (331)
    vector<int32_t> minhkl = {3, 3, 1};
    // Right most peak (200)
    vector<int> rightmostpeakhkl = {2, 0, 0};

    FitPowderDiffPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "FittedPeaks");
    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputBraggPeakParameterWorkspace", "PeaksParameterTable");
    alg.setProperty("OutputZscoreWorkspace", "ZscoreTable");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("MinTOF", 19650.0);
    alg.setProperty("MaxTOF", 49000.0);
    alg.setProperty("MinimumHKL", minhkl);
    alg.setProperty("NumberPeaksToFitBelowLowLimit", 2);
    alg.setProperty("FittingMode", "Robust");
    alg.setProperty("MinimumPeakHeight", 0.5);
    alg.setProperty("RightMostPeakHKL", rightmostpeakhkl);
    alg.setProperty("RightMostPeakLeftBound", 46300.0);
    alg.setProperty("RightMostPeakRightBound", 47903.0);
    alg.execute();
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("DataWorkspace");
    AnalysisDataService::Instance().remove("PeakParameters");
    AnalysisDataService::Instance().remove("InstrumentParameters");
    AnalysisDataService::Instance().remove("FittedPeaks");
    AnalysisDataService::Instance().remove("PeaksParameterTable");
  }

private:
  API::MatrixWorkspace_sptr dataws;
  DataObjects::TableWorkspace_sptr peakparamws;
  DataObjects::TableWorkspace_sptr geomparamws;
};

#endif /* MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_ */
