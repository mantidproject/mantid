#ifndef MANTID_MUON_APPLYMUONDETECTORGROUPPAIRINGTEST_H_
#define MANTID_MUON_APPLYMUONDETECTORGROUPPAIRINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidMuon/ApplyMuonDetectorGroupPairing.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;

namespace {

// Create fake muon data with exponential decay
struct yDataAsymmetry {
  double operator()(const double time, size_t specNum) {
    double amplitude = (static_cast<double>(specNum) + 1) * 10.;
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds
    return (20. * (1.0 + amplitude * exp(-time / tau)));
  }
};

struct yDataCounts {
  yDataCounts() : m_count(-1) {}
  int m_count;
  double operator()(const double, size_t) {
    m_count++;
    return static_cast<double>(m_count);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

/**
 * Create a matrix workspace appropriate for Group Asymmetry. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @return Pointer to the workspace.
 */
MatrixWorkspace_sptr createAsymmetryWorkspace(size_t nspec, size_t maxt) {

  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yDataAsymmetry(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(
      static_cast<int>(nspec)));

  for (int g = 0; g < static_cast<int>(nspec); g++) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
  }

  // Add number of good frames (required for Asymmetry calculation)
  ws->mutableRun().addProperty("goodfrm", 10);

  return ws;
}

/**
 * Create a matrix workspace appropriate for Group Counts. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal. Y values increase from 0 in integer steps.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param seed :: Number added to all y-values.
 * @return Pointer to the workspace.
 */
MatrixWorkspace_sptr createCountsWorkspace(size_t nspec, size_t maxt,
                                           double seed) {

  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yDataCounts(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(
      static_cast<int>(nspec)));

  for (int g = 0; g < static_cast<int>(nspec); g++) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
    ws->mutableY(g) += seed;
  }

  return ws;
}

/**
 * Create a WorkspaceGroup and add to the ADS, populate with MatrixWorkspaces
 * simulating periods as used in muon analysis. Workspace for period i has a
 * name ending _i.
 * @param nPeriods :: The number of periods (independent workspaces)
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param wsGroupName :: Name of the workspace group containing the period
 * workspaces.
 * @return Pointer to the workspace group.
 */
WorkspaceGroup_sptr
createMultiPeriodWorkspaceGroup(const int &nPeriods, size_t nspec, size_t maxt,
                                const std::string &wsGroupName) {

  WorkspaceGroup_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(wsGroupName, wsGroup);

  std::string wsNameStem = "MuonDataPeriod_";
  std::string wsName;

  for (int period = 1; period < nPeriods + 1; period++) {
    // Period 1 yvalues : 1,2,3,4,5,6,7,8,9,10
    // Period 2 yvalues : 2,3,4,5,6,7,8,9,10,11 etc..
    MatrixWorkspace_sptr ws = createCountsWorkspace(nspec, maxt, period);
    wsGroup->addWorkspace(ws);
    wsName = wsNameStem + std::to_string(period);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
  }

  return wsGroup;
}

ITableWorkspace_sptr createDeadTimeTable(const size_t &nspec,
                                         std::vector<double> &deadTimes) {

  auto deadTimeTable = boost::dynamic_pointer_cast<ITableWorkspace>(
      WorkspaceFactory::Instance().createTable("TableWorkspace"));

  deadTimeTable->addColumn("int", "Spectrum Number");
  deadTimeTable->addColumn("double", "Dead Time");

  if (deadTimes.size() != nspec) {
    return deadTimeTable;
  }

  for (size_t spec = 0; spec < deadTimes.size(); spec++) {
    TableRow newRow = deadTimeTable->appendRow();
    newRow << static_cast<int>(spec) + 1;
    newRow << deadTimes[spec];
  }

  return deadTimeTable;
}

} // namespace

class ApplyMuonDetectorGroupPairingTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ApplyMuonDetectorGroupPairingTest() {
    Mantid::API::FrameworkManager::Instance();
  }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyMuonDetectorGroupPairingTest *createSuite() {
    return new ApplyMuonDetectorGroupPairingTest();
  }
  static void destroySuite(ApplyMuonDetectorGroupPairingTest *suite) {
    delete suite;
  }

  void test_Init() {
    ApplyMuonDetectorGroupPairing alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_producesOutputWorkspaces() {}

  void test_inputWorkspaceWithMultipleSpectraFails() {
    // We expect the input workspaces to have a single spectra.
  }

  void test_inputWorkspaceWithDifferentTimeAxisFails() {
    // e.g. rebin with non-rebin should throw an error from this algorithm.
  }

  void test_inputWorkspaceWithDifferentTimeAxisFails() {
    //
  }

  void test_groupsAreEqualFails() {
    // If the groupings are identical then the algorithm should not execute.
  }

  void test_workspacePairingCorrectAsymmetry() {
    // Check hand calculated numbers that the pairing calculation is done
    // correctly
  }

  void test_groupPairingCorrectAsymmetry() {
    // Check that the calculation is done correctly when the detector ID groups
    // are specified in the inputs.
  }

  void test_detectorIDsNotInWorkspaceFails() {
    // If the input detector IDs are not in the input workspace the algorithm
    // should not execute
  }

  void test_correctOutputWorkspaceNames() {
    // Check the sytax of the generated names is what we expect
  }

  void test_multiplePeriodData() {
    // ???
  }

  void test_deadTimeCorrection() {
    // ???
  }



};

#endif /* MANTID_MUON_APPLYMUONDETECTORGROUPPAIRINGTEST_H_ */
