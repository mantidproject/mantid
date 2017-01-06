#ifndef MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_
#define MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ProcessBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/MersenneTwister.h"

#include <fstream>

using Mantid::CurveFitting::Functions::ProcessBackground;
using namespace Mantid;
using namespace Mantid::API;
using namespace Kernel;
using namespace Mantid::DataObjects;
using namespace HistogramData;

namespace {
Workspace2D_sptr createInputWS(std::string name, size_t sizex, size_t sizey) {
  Workspace2D_sptr inputWS = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey));
  AnalysisDataService::Instance().addOrReplace(name, inputWS);

  return inputWS;
}
}

class ProcessBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundTest *createSuite() {
    return new ProcessBackgroundTest();
  }
  static void destroySuite(ProcessBackgroundTest *suite) { delete suite; }

  /** Test option delete region
   */
  void test_DeleteRegion() {
    size_t wsSize = 10;
    auto inpws = createInputWS("Background1", wsSize, wsSize);
    for (size_t i = 0; i < wsSize; ++i) {
      inpws->mutableX(0)[i] = double(i);
      inpws->mutableY(0)[i] = double(i) * double(i);
    }

    // 2. Do the job
    ProcessBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "NewBackground"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "DeleteRegion"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 4.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 6.3));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check
    Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("NewBackground"));
    size_t newsize = outws->x(0).size();

    TS_ASSERT_EQUALS(newsize, 8);

    // 4. Clean
    AnalysisDataService::Instance().remove("Background1");
    AnalysisDataService::Instance().remove("NewBackground");

    return;
  }

  /** Test option "Add Region"
   */
  void test_AddRegion() {
    size_t wsSize = 10;
    auto inpws = createInputWS("Background2", wsSize, wsSize);
    for (size_t i = 0; i < wsSize; ++i) {
      inpws->mutableX(0)[i] = double(i);
      inpws->mutableY(0)[i] = double(i) * double(i);
    }

    auto refws = createInputWS("RefBackground", wsSize, wsSize);
    for (size_t i = 0; i < wsSize; ++i) {
      refws->mutableX(0)[i] = double(i) * 0.3 + 1.01;
      refws->mutableY(0)[i] = double(i) * double(i);
    }

    // 2. Do the job
    ProcessBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inpws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "NewBackground"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReferenceWorkspace", refws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Options", "AddRegion"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LowerBound", 1.001));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpperBound", 1.99));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check
    Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("NewBackground"));
    size_t newsize = outws->x(0).size();

    TS_ASSERT_EQUALS(newsize, 14);

    // 4. Clean
    AnalysisDataService::Instance().remove("Background2");
    AnalysisDataService::Instance().remove("RefBackground");
    AnalysisDataService::Instance().remove("NewBackground");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection
    * Disabled because it requires a data file
   */
  void Passed_test_AutoBackgroundSelection() {

    // 1. Prepare for data
    std::string datafile("/home/wzz/Mantid/Code/debug/MyTestData/4862b7.inp");
    Workspace2D_sptr dataws = createWorkspace2D(datafile);
    AnalysisDataService::Instance().addOrReplace("DiffractionData", dataws);
    /// Background points for bank 7
    std::vector<double> bkgdpts = {57741.0,  63534.0,  69545.0,
                                   89379.0,  89379.0,  115669.0,
                                   134830.0, 165131.0, 226847.0};

    // 2. Prepare algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints");
    alg.setProperty("Options", "SelectBackgroundPoints");

    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("BackgroundPoints", bkgdpts);

    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("NoiseTolerance", 100.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints"));
    TS_ASSERT(bkgdws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection
   */
  void test_SimpleBackgroundGeneration() {
    // Create input data
    size_t wsSize = 1000;
    auto dataws = createInputWS("DiffractionData1", wsSize, wsSize);
    for (size_t i = 0; i < wsSize; ++i) {
      dataws->mutableX(0)[i] = double(i);
      dataws->mutableY(0)[i] = double(i) * double(i);
    }

    std::vector<double> bkgdpts = {577.400, 635.340, 695.450, 893.790};

    // Prepare algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints");
    alg.setProperty("Options", "SelectBackgroundPoints");
    alg.setProperty("BackgroundPointSelectMode",
                    "Input Background Points Only");

    alg.setProperty("SelectionMode", "FitGivenDataPoints");
    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("BackgroundPoints", bkgdpts);

    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("NoiseTolerance", 100.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints"));
    TS_ASSERT(bkgdws);
    if (bkgdws) {
      TS_ASSERT_EQUALS(bkgdws->x(0).size(), bkgdpts.size());
    }

    AnalysisDataService::Instance().remove("DiffractionData1");
    AnalysisDataService::Instance().remove("SelectedBackgroundPoints");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test automatic background selection from a given data
   */
  void test_SelectBackgroundFromInputFunction() {
    // Create input data
    size_t wsSize = 1000;
    auto dataws = createInputWS("DiffractionData2", wsSize, wsSize);
    for (size_t i = 0; i < wsSize; ++i) {
      dataws->mutableX(0)[i] = double(i);
      dataws->mutableY(0)[i] =
          double(i) * double(i) + sin(double(i) / 180. * 3.14);
    }

    // Create background function
    TableWorkspace_sptr functablews = boost::make_shared<TableWorkspace>();
    functablews->addColumn("str", "Name");
    functablews->addColumn("double", "Value");
    TableRow row0 = functablews->appendRow();
    row0 << "A0" << 0.;
    TableRow row1 = functablews->appendRow();
    row1 << "A1" << 0.;
    TableRow row2 = functablews->appendRow();
    row2 << "A2" << 1.;
    AnalysisDataService::Instance().addOrReplace("BackgroundParameters",
                                                 functablews);

    // Create and set up algorithm
    ProcessBackground alg;
    alg.initialize();

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputWorkspace", "SelectedBackgroundPoints2");
    alg.setProperty("Options", "SelectBackgroundPoints");

    alg.setProperty("BackgroundType", "Polynomial");
    alg.setProperty("SelectionMode", "UserFunction");
    alg.setProperty("BackgroundTableWorkspace", functablews);

    alg.setProperty("OutputBackgroundParameterWorkspace",
                    "OutBackgroundParameters");
    alg.setProperty("UserBackgroundWorkspace", "VisualWS");
    alg.setProperty("OutputBackgroundType", "Chebyshev");
    alg.setProperty("OutputBackgroundOrder", 6);

    alg.setProperty("NoiseTolerance", 0.25);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    // 3. Check the result
    Workspace2D_sptr bkgdws = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("SelectedBackgroundPoints2"));
    TS_ASSERT(bkgdws);
    if (bkgdws) {
      TS_ASSERT(bkgdws->x(0).size() > 10);
      TS_ASSERT_EQUALS(bkgdws->getNumberHistograms(), 3);
    }

    TableWorkspace_sptr bkgdparws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("OutBackgroundParameters"));
    TS_ASSERT(bkgdparws);

    AnalysisDataService::Instance().remove("DiffractionData2");
    AnalysisDataService::Instance().remove("SelectedBackgroundPoints2");
    AnalysisDataService::Instance().remove("BackgroundParameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Read column file to create a workspace2D
   */
  Workspace2D_sptr createWorkspace2D(std::string filename) {
    // 1. Read data
    auto data = importDataFromColumnFile(filename);

    // 2. Create workspace
    size_t datasize = data.x().size();
    DataObjects::Workspace2D_sptr dataws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::WorkspaceFactory::Instance().create("Workspace2D", 1, datasize,
                                                     datasize));
    dataws->setHistogram(0, data);

    std::cout << "DBT505  dataX range: " << data.x()[0] << ", "
              << data.x().back() << " sized " << data.x().size() << '\n';

    return dataws;
  }

  /** Import data from a column data file
   */
  Histogram importDataFromColumnFile(std::string filename) {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open()) {
      std::cout << "File " << filename << " cannot be opened. \n";
      throw std::invalid_argument("Unable to open data file. ");
    }

    // 2. Read file
    char line[256];
    std::vector<double> vx, vy, ve;
    while (ins.getline(line, 256)) {
      if (line[0] != '#') {
        double x, y;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y;
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vx.push_back(x);
        vy.push_back(y);
        ve.push_back(e);
      }
    }

    return Histogram(Points(vx), Counts(vy), CountStandardDeviations(ve));
  }
};

// Performance test for "Delete Region" option
class ProcessBackgroundDRTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundDRTestPerformance *createSuite() {
    return new ProcessBackgroundDRTestPerformance();
  }

  static void destroySuite(ProcessBackgroundDRTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    auto inpws = createInputWS("Background1", 1000000, 1000000);
    for (size_t i = 0; i < 1000000; ++i) {
      inpws->mutableX(0)[i] = double(i);
      inpws->mutableY(0)[i] = double(i) * double(i);
    }

    dr.initialize();
    dr.setProperty("InputWorkspace", inpws);
    dr.setProperty("OutputWorkspace", "NewBackground");
    dr.setProperty("Options", "DeleteRegion");
    dr.setProperty("LowerBound", 450000.0);
    dr.setProperty("UpperBound", 630000.0);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("Background1");
  }

  void testPerformanceWS() { dr.execute(); }

private:
  ProcessBackground dr;
};

// Performance test for "Add Region" option
class ProcessBackgroundARTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundARTestPerformance *createSuite() {
    return new ProcessBackgroundARTestPerformance();
  }

  static void destroySuite(ProcessBackgroundARTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    auto inpws = createInputWS("Background2", 80000, 80000);
    for (size_t i = 0; i < 80000; ++i) {
      inpws->mutableX(0)[i] = double(i);
      inpws->mutableY(0)[i] = double(i) * double(i);
    }

    auto refws = createInputWS("RefBackground", 80000, 80000);
    for (size_t i = 0; i < 80000; ++i) {
      refws->mutableX(0)[i] = double(i) * 0.3 + 8080.0;
      refws->mutableY(0)[i] = double(i) * double(i);
    }

    ar.initialize();
    ar.setProperty("InputWorkspace", inpws);
    ar.setProperty("OutputWorkspace", "NewBackground");
    ar.setProperty("ReferenceWorkspace", refws);
    ar.setProperty("Options", "AddRegion");
    ar.setProperty("LowerBound", 8000.0);
    ar.setProperty("UpperBound", 16000.0);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("Background2");
    AnalysisDataService::Instance().remove("RefBackground");
  }

  void testPerformanceWS() { ar.execute(); }

private:
  ProcessBackground ar;
};

// Performance test for Simple Background Generation
class ProcessBackgroundSBGTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundSBGTestPerformance *createSuite() {
    return new ProcessBackgroundSBGTestPerformance();
  }

  static void destroySuite(ProcessBackgroundSBGTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    auto inpws = createInputWS("DiffractionData1", 1000000, 1000000);
    for (size_t i = 0; i < 1000000; ++i) {
      inpws->mutableX(0)[i] = double(i);
      inpws->mutableY(0)[i] = double(i) * double(i);
    }

    MersenneTwister mt(1234, 0.0, 1000000.0);
    std::vector<double> bkgdpts(10000);
    for (auto it = bkgdpts.begin(); it != bkgdpts.end(); it++) {
      *it = mt.nextValue();
    }

    sbg.initialize();
    sbg.setProperty("InputWorkspace", inpws);
    sbg.setProperty("OutputWorkspace", "SelectedBackgroundPoints");
    sbg.setProperty("Options", "SelectBackgroundPoints");
    sbg.setProperty("BackgroundPointSelectMode",
                    "Input Background Points Only");
    sbg.setProperty("SelectionMode", "FitGivenDataPoints");
    sbg.setProperty("BackgroundType", "Polynomial");
    sbg.setProperty("BackgroundPoints", bkgdpts);
    sbg.setProperty("WorkspaceIndex", 0);
    sbg.setProperty("NoiseTolerance", 100.0);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("DiffractionData1");
  }

  void testPerformanceWS() { sbg.execute(); }

private:
  ProcessBackground sbg;
};

// Performance test for Select Background From Input Function
class ProcessBackgroundSBFIFTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessBackgroundSBFIFTestPerformance *createSuite() {
    return new ProcessBackgroundSBFIFTestPerformance();
  }

  static void destroySuite(ProcessBackgroundSBFIFTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    auto dataws = createInputWS("DiffractionData2", 50000, 50000);
    for (size_t i = 0; i < 50000; ++i) {
      dataws->mutableX(0)[i] = double(i);
      dataws->mutableY(0)[i] =
          double(i) * double(i) + sin(double(i) / 180. * 3.14);
    }

    // Create background function
    TableWorkspace_sptr functablews = boost::make_shared<TableWorkspace>();
    functablews->addColumn("str", "Name");
    functablews->addColumn("double", "Value");
    TableRow row0 = functablews->appendRow();
    row0 << "A0" << 0.;
    TableRow row1 = functablews->appendRow();
    row1 << "A1" << 0.;
    TableRow row2 = functablews->appendRow();
    row2 << "A2" << 1.;
    AnalysisDataService::Instance().addOrReplace("BackgroundParameters",
                                                 functablews);

    // Create and set up algorithm
    sbfif.initialize();
    sbfif.setProperty("InputWorkspace", dataws);
    sbfif.setProperty("WorkspaceIndex", 0);
    sbfif.setProperty("OutputWorkspace", "SelectedBackgroundPoints2");
    sbfif.setProperty("Options", "SelectBackgroundPoints");
    sbfif.setProperty("BackgroundType", "Polynomial");
    sbfif.setProperty("SelectionMode", "UserFunction");
    sbfif.setProperty("BackgroundTableWorkspace", functablews);
    sbfif.setProperty("OutputBackgroundParameterWorkspace",
                      "OutBackgroundParameters");
    sbfif.setProperty("UserBackgroundWorkspace", "VisualWS");
    sbfif.setProperty("OutputBackgroundType", "Chebyshev");
    sbfif.setProperty("OutputBackgroundOrder", 6);
    sbfif.setProperty("NoiseTolerance", 0.25);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("DiffractionData2");
    AnalysisDataService::Instance().remove("BackgroundParameters");
  }

  void testPerformanceWS() { sbfif.execute(); }

private:
  ProcessBackground sbfif;
};

#endif /* MANTID_ALGORITHMS_PROCESSBACKGROUNDTEST_H_ */
