#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MuonWorkspaceCreationHelper {

// Generate y-values which increment by 1 each time the function is called
yDataCounts2::yDataCounts2() : m_count(-1) {}
double yDataCounts2::operator()(const double, size_t) {
  m_count++;
  return static_cast<double>(m_count);
}

/**
 * Create y-values representing muon data, each spectrum is offset by 4 degrees
 * in phase and has a different normalization. Counts are capped at zero to
 * prevent negative values.
 */
yDataAsymmetry::yDataAsymmetry(const double amp, const double phi)
    : m_amp(amp), m_phi(phi) {}
yDataAsymmetry::yDataAsymmetry() {
  m_amp = 1.5;
  m_phi = 0.1;
}
double yDataAsymmetry::operator()(const double t, size_t spec) {
  double e = exp(-t / tau);
  double factor = (static_cast<double>(spec) + 1.0) * 0.5;
  double phase_offset = 4 * M_PI / 180;
  return std::max(0.0, (10. * factor * (1.0 +
                                        m_amp * cos(m_omega * t + m_phi +
                                                    static_cast<double>(spec) *
                                                        phase_offset)) *
                        e));
}

// Errors are fixed to 0.005
double eData::operator()(const double, size_t) { return 0.005; }

/**
 * Create a matrix workspace appropriate for Group Asymmetry. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param dataGenerator :: A function object which takes a double (t) and
 * integer (spectrum number) and gives back a double (the y-value for the data).
 * @return Pointer to the workspace.
 */
template <typename Function>
MatrixWorkspace_sptr createAsymmetryWorkspace(std::size_t nspec,
                                              std::size_t maxt,
                                              Function dataGenerator) {
  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          dataGenerator, static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  for (auto g = 0u; g < nspec; ++g) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
  }
  // Add number of good frames (required for Asymmetry calculation)
  ws->mutableRun().addProperty("goodfrm", 10);
  // Add instrument and run number
  boost::shared_ptr<Geometry::Instrument> inst1 =
      boost::make_shared<Geometry::Instrument>();
  inst1->setName("EMU");
  ws->setInstrument(inst1);
  ws->mutableRun().addProperty("run_number", 12345);

  return ws;
}

Mantid::API::MatrixWorkspace_sptr createAsymmetryWorkspace(size_t nspec,
                                                           size_t maxt) {
  return createAsymmetryWorkspace(nspec, maxt, yDataAsymmetry());
}

/**
 * Create a matrix workspace appropriate for Group Counts. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal. Y values increase from 0 in integer steps.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param seed :: Number added to all y-values.
 * @param detectorIDseed :: detector IDs starting from this number.
 * @return Pointer to the workspace.
 */
MatrixWorkspace_sptr createCountsWorkspace(size_t nspec, size_t maxt,
                                           double seed, size_t detectorIDseed) {

  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yDataCounts2(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(
      static_cast<int>(nspec)));

  for (int g = 0; g < static_cast<int>(nspec); g++) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + static_cast<int>(detectorIDseed));
    spec.setSpectrumNo(g + 1);
    ws->mutableY(g) += seed;
  }

  // Add number of good frames (required for Asymmetry calculation)
  ws->mutableRun().addProperty("goodfrm", 10);
  // Add instrument and run number
  boost::shared_ptr<Geometry::Instrument> inst1 =
      boost::make_shared<Geometry::Instrument>();
  inst1->setName("EMU");
  ws->setInstrument(inst1);
  ws->mutableRun().addProperty("run_number", 12345);

  return ws;
}

/**
 * Create a WorkspaceGroup and add to the ADS, populate with MatrixWorkspaces
 * simulating periods as used in muon analysis. Workspace for period i has a
 * name ending _i.
 * @param nPeriods :: The number of periods (independent workspaces).
 * @param nspec :: The number of spectra in each workspace.
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

  boost::shared_ptr<Geometry::Instrument> inst1 =
      boost::make_shared<Geometry::Instrument>();
  inst1->setName("EMU");

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

/**
 * Create a simple dead time TableWorkspace with two columns (spectrum number
 * and dead time).
 * @param nspec :: The number of spectra (rows in table).
 * @param deadTimes ::  The dead times for each spectra.
 * @return TableWorkspace with dead times appropriate for pairing algorithm.
 */
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

/**
 * Creates a single-point workspace with instrument and runNumber set.
 * @param intrName :: Instrument name e.g. MUSR
 * @param runNumber ::  e.g. 1000
 * @param nSpectra :: Number of spectra in the workspace, defaults to 1.
 * @return Pointer to the workspace.
 */
Workspace_sptr createWorkspaceWithInstrumentandRun(const std::string &instrName,
                                                   int runNumber,
                                                   size_t nSpectra) {

  Geometry::Instrument_const_sptr instr =
      boost::make_shared<Geometry::Instrument>(instrName);
  MatrixWorkspace_sptr ws =
      WorkspaceFactory::Instance().create("Workspace2D", nSpectra, 1, 1);
  ws->setInstrument(instr);
  ws->mutableRun().addProperty("run_number", runNumber);
  return ws;
}

/**
 * Creates a grouped workspace containing multiple workspaces with multiple
 * spectra, the detector IDs are consequtive across the spectra, starting from 1
 * @param nWorkspaces :: The number of workspaces.
 * @param nspec :: The number of spectra per workspace.
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param wsGroupName :: Name of the workspace group.
 */
WorkspaceGroup_sptr
createWorkspaceGroupConsecutiveDetectorIDs(const int &nWorkspaces, size_t nspec,
                                           size_t maxt,
                                           const std::string &wsGroupName) {

  WorkspaceGroup_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(wsGroupName, wsGroup);

  std::string wsNameStem = "MuonDataPeriod_";
  std::string wsName;

  for (int period = 1; period < nWorkspaces + 1; period++) {
    // Period 1 yvalues : 1,2,3,4,5,6,7,8,9,10
    // Period 2 yvalues : 2,3,4,5,6,7,8,9,10,11 etc..
    size_t detIDstart = (period - 1) * nspec + 1;
    MatrixWorkspace_sptr ws =
        createCountsWorkspace(nspec, maxt, period, detIDstart);
    wsGroup->addWorkspace(ws);

    wsName = wsNameStem + std::to_string(period);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
  }

  return wsGroup;
}

} // namespace MuonWorkspaceCreationHelper
