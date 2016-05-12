#include "MantidAPI/FileProperty.h"

#include "MantidCrystal/SortHKL.h"

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/Utils.h"

#include <cmath>
#include <fstream>
#include <numeric>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortHKL)

SortHKL::SortHKL() {
  m_pointGroups = getAllPointGroups();
  m_refConds = getAllReflectionConditions();
}

SortHKL::~SortHKL() {}

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

  std::map<V3D, UniqueReflection> uniqueReflections =
      getUniqueReflections(peaks, cell);

  PeaksStatistics peaksStatistics(uniqueReflections, peaks.size());

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
std::map<V3D, UniqueReflection>
SortHKL::getUniqueReflections(const std::vector<Peak> &peaks,
                              const UnitCell &cell) const {
  ReflectionCondition_sptr centering = getCentering();
  PointGroup_sptr pointGroup = getPointgroup();

  std::pair<double, double> dLimits = getDLimits(peaks, cell);

  std::map<V3D, UniqueReflection> uniqueReflectionInRange =
      getPossibleUniqueReflections(cell, dLimits, pointGroup, centering);

  for (auto const &peak : peaks) {
    V3D hkl = peak.getHKL();
    hkl.round();

    uniqueReflectionInRange.at(pointGroup->getReflectionFamily(hkl))
        .addPeak(peak);
  }

  return uniqueReflectionInRange;
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

/**
 * @brief SortHKL::getPossibleUniqueReflections
 *
 * This method returns a map that contains UniqueReflection-objects, one
 * for each unique reflection in the given resolution range. It uses the
 * given cell, point group and centering to determine which reflections
 * are allowed and which ones are equivalent.
 *
 * @param cell :: UnitCell of the sample.
 * @param dLimits :: Resolution limits for the generated reflections.
 * @param pointGroup :: Point group of the sample.
 * @param centering :: Lattice centering (important for completeness
 * calculation).
 *
 * @return Map of UniqueReflection objects with HKL of the reflection family as
 * key
 */
std::map<V3D, UniqueReflection> SortHKL::getPossibleUniqueReflections(
    const UnitCell &cell, const std::pair<double, double> &dLimits,
    const PointGroup_sptr &pointGroup,
    const ReflectionCondition_sptr &centering) const {

  HKLGenerator generator(cell, dLimits.first);
  HKLFilter_const_sptr dFilter = boost::make_shared<const HKLFilterDRange>(
      cell, dLimits.first, dLimits.second);
  HKLFilter_const_sptr centeringFilter =
      boost::make_shared<const HKLFilterCentering>(centering);
  HKLFilter_const_sptr filter = dFilter & centeringFilter;

  // Generate map of UniqueReflection-objects with reflection family as key.
  std::map<V3D, UniqueReflection> uniqueHKLs;
  for (const auto &hkl : generator) {
    if (filter->isAllowed(hkl)) {
      V3D hklFamily = pointGroup->getReflectionFamily(hkl);
      uniqueHKLs.emplace(hklFamily, UniqueReflection(hklFamily));
    }
  }

  return uniqueHKLs;
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

  newrow << name << statistics.m_uniqueReflections << statistics.m_lambdaMin
         << statistics.m_lambdaMax << statistics.m_redundancy
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

/// Returns the sum of all I/sigma-ratios defined by the two vectors using
/// std::inner_product.
double PeaksStatistics::getIOverSigmaSum(
    const std::vector<double> &sigmas,
    const std::vector<double> &intensities) const {
  return std::inner_product(intensities.begin(), intensities.end(),
                            sigmas.begin(), 0.0, std::plus<double>(),
                            std::divides<double>());
}

/// Returns the Root mean square of the supplied vector.
double PeaksStatistics::getRMS(const std::vector<double> &data) const {
  double sumOfSquares =
      std::inner_product(data.begin(), data.end(), data.begin(), 0.0);

  return sqrt(sumOfSquares / static_cast<double>(data.size()));
}

/// Returns the lowest and hights wavelength in the peak list.
std::pair<double, double>
PeaksStatistics::getLambdaLimits(const std::vector<Peak> &peaks) const {
  if (peaks.empty()) {
    return std::make_pair(0.0, 0.0);
  }

  auto lambdaLimitIterators = std::minmax_element(
      peaks.begin(), peaks.end(), [](const Peak &lhs, const Peak &rhs) {
        return lhs.getWavelength() < rhs.getWavelength();
      });

  return std::make_pair((*(lambdaLimitIterators.first)).getWavelength(),
                        (*(lambdaLimitIterators.second)).getWavelength());
}

/// Sorts the peaks in the workspace by H, K and L.
void SortHKL::sortOutputPeaksByHKL(IPeaksWorkspace_sptr outputPeaksWorkspace) {
  // Sort by HKL
  std::vector<std::pair<std::string, bool>> criteria{
      {"H", true}, {"K", true}, {"L", true}};
  outputPeaksWorkspace->sort(criteria);
}

/**
 * @brief PeaksStatistics::calculatePeaksStatistics
 *
 * This function iterates through the unique reflections map and computes
 * statistics for the reflections/peaks. It calls
 * UniqueReflection::removeOutliers, so outliers are removed before the
 * statistical quantities are calculated.
 *
 * Furthermore it sets the intensities of each peak to the mean of the
 * group of equivalent reflections.
 *
 * @param uniqueReflections :: Map of unique reflections and peaks.
 */
void PeaksStatistics::calculatePeaksStatistics(
    std::map<V3D, UniqueReflection> &uniqueReflections) {
  double rMergeNumerator = 0.0;
  double rPimNumerator = 0.0;
  double intensitySumRValues = 0.0;
  double iOverSigmaSum = 0.0;

  for (auto &unique : uniqueReflections) {
    /* Since all possible unique reflections are explored
     * there may be 0 observations for some of them.
     * In that case, nothing can be done.*/
    if (unique.second.count() > 0) {
      ++m_uniqueReflections;

      // Possibly remove outliers.
      unique.second.removeOutliers();

      // I/sigma is calculated for all reflections, even if there is only one
      // observation.
      const std::vector<double> &intensities = unique.second.getIntensities();
      const std::vector<double> &sigmas = unique.second.getSigmas();

      // Accumulate the I/sigma's for current reflection into sum
      iOverSigmaSum += getIOverSigmaSum(sigmas, intensities);

      if (unique.second.count() > 1) {
        // Get mean, standard deviation for intensities
        Statistics intensityStatistics = Kernel::getStatistics(
            intensities, StatOptions::Mean | StatOptions::UncorrectedStdDev);

        double meanIntensity = intensityStatistics.mean;

        /* This was in the original algorithm, not entirely sure where it is
         * used. It's basically the sum of all relative standard deviations.
         * In a perfect data set with all equivalent reflections exactly
         * equivalent that would be 0. */
        m_chiSquared += intensityStatistics.standard_deviation / meanIntensity;

        // For both RMerge and RPim sum(|I - <I>|) is required
        double sumOfDeviationsFromMean =
            std::accumulate(intensities.begin(), intensities.end(), 0.0,
                            [meanIntensity](double sum, double intensity) {
                              return sum + fabs(intensity - meanIntensity);
                            });

        // Accumulate into total sum for numerator of RMerge
        rMergeNumerator += sumOfDeviationsFromMean;

        // For Rpim, the sum is weighted by a factor depending on N
        double rPimFactor =
            sqrt(1.0 / (static_cast<double>(unique.second.count()) - 1.0));
        rPimNumerator += (rPimFactor * sumOfDeviationsFromMean);

        // Collect sum of intensities for R-value calculation
        intensitySumRValues +=
            std::accumulate(intensities.begin(), intensities.end(), 0.0);

        // The original algorithm sets the intensities and sigmas to the mean.
        double sqrtOfMeanSqrSigma = getRMS(sigmas);
        unique.second.setPeaksIntensityAndSigma(meanIntensity,
                                                sqrtOfMeanSqrSigma);
      }

      const std::vector<Peak> &reflectionPeaks = unique.second.getPeaks();
      m_peaks.insert(m_peaks.end(), reflectionPeaks.begin(),
                     reflectionPeaks.end());
    }
  }

  m_measuredReflections = static_cast<int>(m_peaks.size());

  if (m_uniqueReflections > 0) {
    m_redundancy = static_cast<double>(m_measuredReflections) /
                   static_cast<double>(m_uniqueReflections);
  }

  m_completeness = static_cast<double>(m_uniqueReflections) /
                   static_cast<double>(uniqueReflections.size());

  if (intensitySumRValues > 0.0) {
    m_rMerge = rMergeNumerator / intensitySumRValues;
    m_rPim = rPimNumerator / intensitySumRValues;
  }

  if (m_measuredReflections > 0) {
    m_meanIOverSigma =
        iOverSigmaSum / static_cast<double>(m_measuredReflections);

    std::pair<double, double> lambdaLimits = getLambdaLimits(m_peaks);
    m_lambdaMin = lambdaLimits.first;
    m_lambdaMax = lambdaLimits.second;
  }
}

/// Returns a vector with the intensities of the Peaks stored in this
/// reflection.
std::vector<double> UniqueReflection::getIntensities() const {
  std::vector<double> intensities;
  intensities.reserve(m_peaks.size());

  std::transform(
      m_peaks.begin(), m_peaks.end(), std::back_inserter(intensities),
      [](const DataObjects::Peak &peak) { return peak.getIntensity(); });

  return intensities;
}

/// Returns a vector with the intensity sigmas of the Peaks stored in this
/// reflection.
std::vector<double> UniqueReflection::getSigmas() const {
  std::vector<double> sigmas;
  sigmas.reserve(m_peaks.size());

  std::transform(
      m_peaks.begin(), m_peaks.end(), std::back_inserter(sigmas),
      [](const DataObjects::Peak &peak) { return peak.getSigmaIntensity(); });

  return sigmas;
}

/// Removes peaks whose intensity deviates more than sigmaCritical from the
/// intensities' mean.
void UniqueReflection::removeOutliers(double sigmaCritical) {
  if (sigmaCritical <= 0.0) {
    throw std::invalid_argument(
        "Critical sigma value has to be greater than 0.");
  }

  if (m_peaks.size() > 2) {
    const std::vector<double> &intensities = getIntensities();
    const std::vector<double> &zScores = Kernel::getZscore(intensities);

    std::vector<size_t> outlierIndices;
    for (size_t i = 0; i < zScores.size(); ++i) {
      if (zScores[i] > sigmaCritical) {
        outlierIndices.push_back(i);
      }
    }

    if (!outlierIndices.empty()) {
      for (auto it = outlierIndices.rbegin(); it != outlierIndices.rend();
           ++it) {
        m_peaks.erase(m_peaks.begin() + (*it));
      }
    }
  }
}

/// Sets the intensities and sigmas of all stored peaks to the supplied values.
void UniqueReflection::setPeaksIntensityAndSigma(double intensity,
                                                 double sigma) {
  for (auto &peak : m_peaks) {
    peak.setIntensity(intensity);
    peak.setSigmaIntensity(sigma);
  }
}

} // namespace Mantid
} // namespace Crystal
