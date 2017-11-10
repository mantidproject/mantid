#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"

#include "MantidCrystal/SortHKL.h"

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Utils.h"

#include <cmath>
#include <fstream>
#include <numeric>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Crystal::PeakStatisticsTools;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortHKL)

SortHKL::SortHKL() {
  m_pointGroups = getAllPointGroups();
  m_refConds = getAllReflectionConditions();
}

SortHKL::~SortHKL() = default;

void SortHKL::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace with an instrument.");

  /* TODO: These two properties with string lists keep appearing -
   * Probably there should be a dedicated Property type or validator. */
  std::vector<std::string> pgOptions;
  pgOptions.reserve(m_pointGroups.size());
  for (auto &pointGroup : m_pointGroups)
    pgOptions.push_back(pointGroup->getName());
  declareProperty("PointGroup", pgOptions[0],
                  boost::make_shared<StringListValidator>(pgOptions),
                  "Which point group applies to this crystal?");

  std::vector<std::string> centeringOptions;
  centeringOptions.reserve(m_refConds.size());
  for (auto &refCond : m_refConds)
    centeringOptions.push_back(refCond->getName());
  declareProperty("LatticeCentering", centeringOptions[0],
                  boost::make_shared<StringListValidator>(centeringOptions),
                  "Appropriate lattice centering for the peaks.");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty("OutputChi2", 0.0, "Chi-square is available as output",
                  Direction::Output);
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "RowName", "Overall", Direction::Input),
                  "name of row");
  declareProperty("Append", false,
                  "Append to output table workspace if true.\n"
                  "If false, new output table workspace (default).");
}

void SortHKL::exec() {
  PeaksWorkspace_sptr inputPeaksWorkspace = getProperty("InputWorkspace");

  const std::vector<Peak> &inputPeaks = inputPeaksWorkspace->getPeaks();
  std::vector<Peak> peaks = getNonZeroPeaks(inputPeaks);

  if (peaks.empty()) {
    g_log.error() << "Number of peaks should not be 0 for SortHKL.\n";
    return;
  }

  UnitCell cell = inputPeaksWorkspace->sample().getOrientedLattice();

  UniqueReflectionCollection uniqueReflections =
      getUniqueReflections(peaks, cell);

  PeaksStatistics peaksStatistics(uniqueReflections);

  // Store the statistics for output.
  const std::string tableName = getProperty("StatisticsTable");
  ITableWorkspace_sptr statisticsTable = getStatisticsTable(tableName);
  insertStatisticsIntoTable(statisticsTable, peaksStatistics);

  // Store all peaks that were used to calculate statistics.

  PeaksWorkspace_sptr outputPeaksWorkspace =
      getOutputPeaksWorkspace(inputPeaksWorkspace);

  std::vector<Peak> &originalOutputPeaks = outputPeaksWorkspace->getPeaks();
  originalOutputPeaks.swap(peaksStatistics.m_peaks);

  sortOutputPeaksByHKL(outputPeaksWorkspace);

  setProperty("OutputWorkspace", outputPeaksWorkspace);
  setProperty("OutputChi2", peaksStatistics.m_chiSquared);
  setProperty("StatisticsTable", statisticsTable);
  AnalysisDataService::Instance().addOrReplace(tableName, statisticsTable);
}

/// Returns a vector which contains only peaks with I > 0, sigma > 0 and valid
/// HKL.
std::vector<Peak>
SortHKL::getNonZeroPeaks(const std::vector<Peak> &inputPeaks) const {
  std::vector<Peak> peaks;
  peaks.reserve(inputPeaks.size());

  std::remove_copy_if(inputPeaks.begin(), inputPeaks.end(),
                      std::back_inserter(peaks), [](const Peak &peak) {
                        return peak.getIntensity() <= 0.0 ||
                               peak.getSigmaIntensity() <= 0.0 ||
                               peak.getHKL() == V3D(0, 0, 0);
                      });

  return peaks;
}

/**
 * @brief SortHKL::getUniqueReflections
 *
 * This method returns a map that contains a UniqueReflection-
 * object with 0 to n Peaks each. The key of the map is the
 * reflection index all peaks are equivalent to.
 *
 * @param peaks :: Vector of peaks to assign.
 * @param cell :: UnitCell to use for calculation of possible reflections.
 * @return Map of unique reflections.
 */
UniqueReflectionCollection
SortHKL::getUniqueReflections(const std::vector<Peak> &peaks,
                              const UnitCell &cell) const {
  ReflectionCondition_sptr centering = getCentering();
  PointGroup_sptr pointGroup = getPointgroup();

  std::pair<double, double> dLimits = getDLimits(peaks, cell);

  UniqueReflectionCollection reflections(cell, dLimits, pointGroup, centering);
  reflections.addObservations(peaks);

  return reflections;
}

/// Returns the centering extracted from the user-supplied property.
ReflectionCondition_sptr SortHKL::getCentering() const {
  ReflectionCondition_sptr centering =
      boost::make_shared<ReflectionConditionPrimitive>();

  std::string refCondName = getPropertyValue("LatticeCentering");
  for (const auto &refCond : m_refConds)
    if (refCond->getName() == refCondName)
      centering = refCond;

  return centering;
}

/// Returns the PointGroup-object constructed from the property supplied by the
/// user.
PointGroup_sptr SortHKL::getPointgroup() const {
  PointGroup_sptr pointGroup =
      PointGroupFactory::Instance().createPointGroup("-1");

  std::string pointGroupName = getPropertyValue("PointGroup");
  for (const auto &m_pointGroup : m_pointGroups)
    if (m_pointGroup->getName() == pointGroupName)
      pointGroup = m_pointGroup;

  return pointGroup;
}

/// Returns the lowest and highest d-Value in the list. Uses UnitCell and HKL
/// for calculation to prevent problems with potentially inconsistent d-Values
/// in Peak.
std::pair<double, double> SortHKL::getDLimits(const std::vector<Peak> &peaks,
                                              const UnitCell &cell) const {
  auto dLimitIterators = std::minmax_element(
      peaks.begin(), peaks.end(), [cell](const Peak &lhs, const Peak &rhs) {
        return cell.d(lhs.getHKL()) < cell.d(rhs.getHKL());
      });

  return std::make_pair(cell.d((*dLimitIterators.first).getHKL()),
                        cell.d((*dLimitIterators.second).getHKL()));
}

/// Create a TableWorkspace for the statistics with appropriate columns or get
/// one from the ADS.
ITableWorkspace_sptr
SortHKL::getStatisticsTable(const std::string &name) const {
  TableWorkspace_sptr tablews;

  // Init or append to a table workspace
  bool append = getProperty("Append");
  if (append && AnalysisDataService::Instance().doesExist(name)) {
    tablews = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(name);
  } else {
    tablews = boost::make_shared<TableWorkspace>();
    tablews->addColumn("str", "Resolution Shell");
    tablews->addColumn("int", "No. of Unique Reflections");
    tablews->addColumn("double", "Resolution Min");
    tablews->addColumn("double", "Resolution Max");
    tablews->addColumn("double", "Multiplicity");
    tablews->addColumn("double", "Mean ((I)/sd(I))");
    tablews->addColumn("double", "Rmerge");
    tablews->addColumn("double", "Rpim");
    tablews->addColumn("double", "Data Completeness");
  }

  return tablews;
}

/// Inserts statistics the supplied PeaksStatistics-objects into the supplied
/// TableWorkspace.
void SortHKL::insertStatisticsIntoTable(
    const ITableWorkspace_sptr &table,
    const PeaksStatistics &statistics) const {
  if (!table) {
    throw std::runtime_error("Can't store statistics into Null-table.");
  }

  std::string name = getProperty("RowName");
  double completeness = 0.0;
  if (name.substr(0, 4) != "bank") {
    completeness = static_cast<double>(statistics.m_completeness);
  }

  // append to the table workspace
  API::TableRow newrow = table->appendRow();

  newrow << name << statistics.m_uniqueReflections << statistics.m_dspacingMin
         << statistics.m_dspacingMax << statistics.m_redundancy
         << statistics.m_meanIOverSigma << 100.0 * statistics.m_rMerge
         << 100.0 * statistics.m_rPim << 100.0 * completeness;
}

/// Returns a PeaksWorkspace which is either the input workspace or a clone.
PeaksWorkspace_sptr SortHKL::getOutputPeaksWorkspace(
    const PeaksWorkspace_sptr &inputPeaksWorkspace) const {
  PeaksWorkspace_sptr outputPeaksWorkspace = getProperty("OutputWorkspace");
  if (outputPeaksWorkspace != inputPeaksWorkspace) {
    outputPeaksWorkspace = inputPeaksWorkspace->clone();
  }

  return outputPeaksWorkspace;
}

/// Sorts the peaks in the workspace by H, K and L.
void SortHKL::sortOutputPeaksByHKL(IPeaksWorkspace_sptr outputPeaksWorkspace) {
  // Sort by HKL
  std::vector<std::pair<std::string, bool>> criteria{
      {"H", true}, {"K", true}, {"L", true}};
  outputPeaksWorkspace->sort(criteria);
}

} // namespace Mantid
} // namespace Crystal
