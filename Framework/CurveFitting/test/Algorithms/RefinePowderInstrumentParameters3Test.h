// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters3.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <fstream>

using Mantid::CurveFitting::Algorithms::RefinePowderInstrumentParameters3;

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace {

//----------------------------------------------------------------------------------------------
/** Generate workspace holding peak positions
 */
Workspace2D_sptr generatePeakPositionWorkspace(int bankid) {

  if (bankid != 1) {
    throw runtime_error("generatePeakPositionWorkspace supports bank 1 only.");
  }

  // 1. Generate vectors, bank 1's peak positions
  const size_t size = 16;
  std::array<double, size> vecDsp = {{0.907108, 0.929509, 0.953656, 0.979788, 1.008190, 1.039220, 1.110980, 1.152910,
                                      1.199990, 1.253350, 1.314520, 1.385630, 1.469680, 1.697040, 1.859020, 2.078440}};
  std::array<double, size> vecTof = {
      {20487.600000, 20994.700000, 21537.400000, 22128.800000, 22769.200000, 23469.400000, 25083.600000, 26048.100000,
       27097.600000, 28272.200000, 29684.700000, 31291.500000, 33394.000000, 38326.300000, 41989.800000, 46921.700000}};
  std::array<double, size> vecError = {{0.350582, 0.597347, 0.644844, 0.879349, 0.417830, 0.481466, 0.527287, 0.554732,
                                        0.363456, 0.614706, 0.468477, 0.785721, 0.555938, 0.728131, 0.390796,
                                        0.997644}};

  // 2. Generate workspace
  Workspace2D_sptr dataws =
      std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

  // 3. Put data
  auto &vecX = dataws->mutableX(0);
  auto &vecY = dataws->mutableY(0);
  auto &vecE = dataws->mutableE(0);
  for (size_t i = 0; i < size; ++i) {
    vecX[i] = vecDsp[i];
    vecY[i] = vecTof[i];
    vecE[i] = vecError[i];
  }

  return dataws;
}

//----------------------------------------------------------------------------------------------
/** Generate a table workspace for holding instrument parameters for POWGEN's
 * bank 1
 */
TableWorkspace_sptr generateInstrumentProfileTableBank1() {
  DataObjects::TableWorkspace *tablews = new DataObjects::TableWorkspace();
  DataObjects::TableWorkspace_sptr geomws = DataObjects::TableWorkspace_sptr(tablews);

  tablews->addColumn("str", "Name");
  tablews->addColumn("double", "Value");
  tablews->addColumn("str", "FitOrTie");
  tablews->addColumn("double", "Min");
  tablews->addColumn("double", "Max");
  tablews->addColumn("double", "StepSize");

  API::TableRow newrow = geomws->appendRow();
  newrow << "Dtt1" << 22778.3 << "f" << 0.0 << 1.0E20 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Dtt1t" << 22747.4 << "t" << 0.0 << 1.0E20 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Dtt2" << 0.0 << "t" << 0.0 << 1.0E20 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Dtt2t" << 0.3 << "t" << -10000.0 << 100000.0 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Tcross" << 0.356 << "t" << 0.0 << 1000.0 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Width" << 1.1072 << "f" << 0.0 << 1000.0 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Zero" << 0.0 << "f" << -10000.0 << 10000.0 << 1.0;

  newrow = geomws->appendRow();
  newrow << "Zerot" << 90.7 << "t" << -10000.0 << 10000.0 << 1.0;

  return geomws;
}

//----------------------------------------------------------------------------------------------
/** Parse Table Workspace to a map of string, double pair
 */
void parseParameterTableWorkspace(const TableWorkspace_sptr &paramws, map<string, double> &paramvalues) {
  for (size_t irow = 0; irow < paramws->rowCount(); ++irow) {
    Mantid::API::TableRow row = paramws->getRow(irow);
    std::string parname;
    double parvalue;
    row >> parname >> parvalue;

    paramvalues.emplace(parname, parvalue);
  }

  return;
}
} // namespace

class RefinePowderInstrumentParameters3Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RefinePowderInstrumentParameters3Test *createSuite() { return new RefinePowderInstrumentParameters3Test(); }
  static void destroySuite(RefinePowderInstrumentParameters3Test *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Fit with non Monte Carlo method.
   * The parameters to fit include Dtt1, Zero, and Width/Tcross
   */
  void test_FitNonMonteCarlo() {
    // 1. Create workspaces for testing
    int bankid = 1;

    // a) Generate workspaces
    Workspace2D_sptr posWS = generatePeakPositionWorkspace(bankid);
    TableWorkspace_sptr profWS = generateInstrumentProfileTableBank1();

    // z) Set to data service
    AnalysisDataService::Instance().addOrReplace("Bank1PeakPositions", posWS);
    AnalysisDataService::Instance().addOrReplace("Bank1ProfileParameters", profWS);

    // 2. Initialization
    RefinePowderInstrumentParameters3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // 3. Set parameters
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");

    alg.setProperty("InputInstrumentParameterWorkspace", "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace", "Bank1FittedProfileParameters");

    alg.setProperty("RefinementAlgorithm", "OneStepFit");
    alg.setProperty("StandardError", "UseInputValue");

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Check result
    // a) Profile parameter table
    TableWorkspace_sptr newgeomparamws = std::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("Bank1FittedProfileParameters"));
    TS_ASSERT(newgeomparamws);
    if (newgeomparamws) {
      std::map<std::string, double> fitparamvalues;
      parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
      TS_ASSERT_DELTA(fitparamvalues["Chi2_Init"], 118348, 1);
      TS_ASSERT_DELTA(fitparamvalues["Chi2_Result"], 4896, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt1"], 22610, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt1t"], 22747, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt2"], 0, 0);
      TS_ASSERT_DELTA(fitparamvalues["Dtt2t"], 0.3, 0.1);
      TS_ASSERT_DELTA(fitparamvalues["Tcross"], 0.356, 0.010);
      TS_ASSERT_DELTA(fitparamvalues["Width"], 370, 1);
      TS_ASSERT_DELTA(fitparamvalues["Zero"], -23.4, 0.1);
      TS_ASSERT_DELTA(fitparamvalues["Zerot"], 90.7, 0.1);
    }

    // b) Data
    Workspace2D_sptr outdataws =
        std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("Bank1FittedPositions"));
    TS_ASSERT(outdataws);

    // 4. Clean
    AnalysisDataService::Instance().remove("Bank1PeakPositions");
    AnalysisDataService::Instance().remove("Bank1FittedPositions");
    AnalysisDataService::Instance().remove("Bank1ProfileParameters");
    AnalysisDataService::Instance().remove("Bank1FittedProfileParameters");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit with Monte Carlo method.
   * The parameters to fit include Dtt1, Zero, and Width/Tcross
   */
  void test_FitMonteCarlo() {
    // 1. Create workspaces for testing
    int bankid = 1;

    // a) Generate workspaces
    Workspace2D_sptr posWS = generatePeakPositionWorkspace(bankid);
    TableWorkspace_sptr profWS = generateInstrumentProfileTableBank1();

    // z) Set to data service
    AnalysisDataService::Instance().addOrReplace("Bank1PeakPositions", posWS);
    AnalysisDataService::Instance().addOrReplace("Bank1ProfileParameters", profWS);

    // 2. Initialization
    RefinePowderInstrumentParameters3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // 3. Set parameters
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");

    alg.setProperty("InputInstrumentParameterWorkspace", "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace", "Bank1FittedProfileParameters");

    alg.setProperty("RefinementAlgorithm", "MonteCarlo");
    alg.setProperty("StandardError", "UseInputValue");

    alg.setProperty("AnnealingTemperature", 100.0);

    alg.setProperty("MonteCarloIterations", 100);

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Check result
    // a) Profile parameter table
    TableWorkspace_sptr newgeomparamws = std::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("Bank1FittedProfileParameters"));
    TS_ASSERT(newgeomparamws);
    if (newgeomparamws) {
      std::map<std::string, double> fitparamvalues;
      parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
      TS_ASSERT_DELTA(fitparamvalues["Chi2_Init"], 118348, 1);
      TS_ASSERT_DELTA(fitparamvalues["Chi2_Result"], 127307, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt1"], 22778, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt1t"], 22747, 1);
      TS_ASSERT_DELTA(fitparamvalues["Dtt2"], 0, 0);
      TS_ASSERT_DELTA(fitparamvalues["Dtt2t"], 0.3, 0.1);
      TS_ASSERT_DELTA(fitparamvalues["Tcross"], 0.356, 0.010);
      TS_ASSERT_DELTA(fitparamvalues["Width"], 1.1, 0.1);
      TS_ASSERT_DELTA(fitparamvalues["Zero"], 0, 0.1);
      TS_ASSERT_DELTA(fitparamvalues["Zerot"], 90.7, 0.1);
    }

    // b) Data
    Workspace2D_sptr outdataws =
        std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("Bank1FittedPositions"));
    TS_ASSERT(outdataws);

    // 4. Clean
    AnalysisDataService::Instance().remove("Bank1PeakPositions");
    AnalysisDataService::Instance().remove("Bank1FittedPositions");
    AnalysisDataService::Instance().remove("Bank1ProfileParameters");
    AnalysisDataService::Instance().remove("Bank1FittedProfileParameters");

    return;
  }
};

using CxxTest::TestSuite;
class RefinePowderInstParams3TestPerformance : public TestSuite {

private:
  Workspace2D_sptr posWS;
  TableWorkspace_sptr profWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RefinePowderInstParams3TestPerformance *createSuite() { return new RefinePowderInstParams3TestPerformance(); }

  static void destroySuite(RefinePowderInstParams3TestPerformance *suite) { delete suite; }

  void setUp() override {
    posWS = generatePeakPositionWorkspace(1);
    profWS = generateInstrumentProfileTableBank1();

    AnalysisDataService::Instance().addOrReplace("Bank1PeakPositions", posWS);
    AnalysisDataService::Instance().addOrReplace("Bank1ProfileParameters", profWS);
  }

  void tearDown() override {

    AnalysisDataService::Instance().remove("Bank1PeakPositions");
    AnalysisDataService::Instance().remove("Bank1FittedPositions");
    AnalysisDataService::Instance().remove("Bank1ProfileParameters");
    AnalysisDataService::Instance().remove("Bank1FittedProfileParameters");
  }

  void test_FitNonMonteCarlo() {

    RefinePowderInstrumentParameters3 alg;
    alg.initialize();
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");
    alg.setProperty("InputInstrumentParameterWorkspace", "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace", "Bank1FittedProfileParameters");
    alg.setProperty("RefinementAlgorithm", "OneStepFit");
    alg.setProperty("StandardError", "UseInputValue");
    alg.execute();
  }

  void test_FitMonteCarlo() {

    RefinePowderInstrumentParameters3 alg;
    alg.initialize();
    alg.setPropertyValue("InputPeakPositionWorkspace", "Bank1PeakPositions");
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("OutputPeakPositionWorkspace", "Bank1FittedPositions");
    alg.setProperty("InputInstrumentParameterWorkspace", "Bank1ProfileParameters");
    alg.setProperty("OutputInstrumentParameterWorkspace", "Bank1FittedProfileParameters");
    alg.setProperty("RefinementAlgorithm", "MonteCarlo");
    alg.setProperty("StandardError", "UseInputValue");
    alg.setProperty("AnnealingTemperature", 100.0);
    alg.setProperty("MonteCarloIterations", 100);
    alg.execute();
  }
};
