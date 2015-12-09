#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortHKL)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SortHKL::SortHKL() {
  m_pointGroups = getAllPointGroups();
  m_refConds = getAllReflectionConditions();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SortHKL::~SortHKL() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SortHKL::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> pgOptions;
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    pgOptions.push_back(m_pointGroups[i]->getName());
  declareProperty("PointGroup", pgOptions[0],
                  boost::make_shared<StringListValidator>(pgOptions),
                  "Which point group applies to this crystal?");

  std::vector<std::string> centeringOptions;
  for (size_t i = 0; i < m_refConds.size(); ++i)
    centeringOptions.push_back(m_refConds[i]->getName());
  declareProperty("LatticeCentering", centeringOptions[0],
                  boost::make_shared<StringListValidator>(centeringOptions),
                  "Appropriate lattice centering for the peaks.");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty("OutputChi2", 0.0, "Chi-square is available as output",
                  Direction::Output);
  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  declareProperty(new PropertyWithValue<std::string>("RowName", "Overall",
                                                     Direction::Input),
                  "name of row");
  declareProperty("Append", false,
                  "Append to output table workspace if true.\n"
                  "If false, new output table workspace (default).");
}

ReflectionCondition_sptr SortHKL::getCentering() const {
  ReflectionCondition_sptr centering =
      boost::make_shared<ReflectionConditionPrimitive>();
  // Get it from the property
  std::string refCondName = getPropertyValue("LatticeCentering");
  for (size_t i = 0; i < m_refConds.size(); ++i)
    if (m_refConds[i]->getName() == refCondName)
      centering = m_refConds[i];

  return centering;
}

PointGroup_sptr SortHKL::getPointgroup() const {
  PointGroup_sptr pointGroup =
      PointGroupFactory::Instance().createPointGroup("-1");
  // Get it from the property
  std::string pointGroupName = getPropertyValue("PointGroup");
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    if (m_pointGroups[i]->getName() == pointGroupName)
      pointGroup = m_pointGroups[i];

  return pointGroup;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
double SortHKL::getIOverSigmaSum(const std::vector<double> &sigmas,
                                 const std::vector<double> &intensities) const {
  std::vector<double> iOverSigma;
  iOverSigma.reserve(sigmas.size());
  std::transform(intensities.begin(), intensities.end(), sigmas.begin(),
                 std::back_inserter(iOverSigma), std::divides<double>());

  return std::accumulate(iOverSigma.begin(), iOverSigma.end(), 0.0);
}

double SortHKL::getMeanOfSquared(const std::vector<double> &data) const {
  double sumOfSquares = std::accumulate(
      data.begin(), data.end(), 0.0,
      [](double sum, double value) { return sum + value * value; });

  return sumOfSquares / static_cast<double>(data.size());
}

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

std::pair<double, double>
SortHKL::getLambdaLimits(const IPeaksWorkspace_sptr &peaksWs) const {
  // Sort by wavelength
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.push_back(std::make_pair("wavelength", true));
  peaksWs->sort(criteria);

  return ResolutionLimits(
      peaksWs->getPeak(0).getWavelength(),
      peaksWs->getPeak(peaksWs->getNumberPeaks() - 1).getWavelength());
}

/**
 * @brief SortHKL::getUniqueReflections
 * @param peaks
 * @param outputPeaksWorkspace
 * @return
 */
std::map<V3D, UniqueReflection> SortHKL::getUniqueReflections(
    const std::vector<Peak> &peaks,
    const IPeaksWorkspace_sptr &outputPeaksWorkspace) const {
  ReflectionCondition_sptr centering = getCentering();
  PointGroup_sptr pointGroup = getPointgroup();

  std::pair<double, double> dLimits =
      getResolutionLimitsD(outputPeaksWorkspace);

  std::map<V3D, UniqueReflection> uniqueReflectionInRange =
      getPossibleUniqueReflections(dLimits.pointGroup, centering);

  for (auto const &peak : peaks) {
    V3D hkl = peak.getHKL();
    hkl.round();

    uniqueReflectionInRange.at(pointGroup->getReflectionFamily(hkl))
        .addPeak(peak);
  }

  return uniqueReflectionInRange;
}

void SortHKL::sortOutputPeaksByHKL(IPeaksWorkspace_sptr outputPeaksWorkspace) {
  std::vector<std::pair<std::string, bool>> criteria;
  // Sort by HKL
  criteria.push_back(std::make_pair("H", true));
  criteria.push_back(std::make_pair("K", true));
  criteria.push_back(std::make_pair("L", true));
  outputPeaksWorkspace->sort(criteria);
}

void SortHKL::exec() {
  PeaksWorkspace_sptr InPeaksW = getProperty("InputWorkspace");

  PeaksWorkspace_sptr outputPeaksWorkspace = getProperty("OutputWorkspace");
  if (outputPeaksWorkspace != InPeaksW)
    outputPeaksWorkspace.reset(InPeaksW->clone().release());

  // Get reference to peaks vector in PeaksWorkspace
  const std::vector<Peak> &peaks = outputPeaksWorkspace->getPeaks();

  // Remove all non-zero peaks
  peaks.erase(std::remove_if(peaks.begin(), peaks.end(), [](const Peak &peak) {
    return peak.getIntensity() <= 0.0 || peak.getSigmaIntensity() <= 0.0 ||
           peak.getHKL() == V3D(0, 0, 0);
  }), peaks.end());

  if (peaks.size() == 0) {
    g_log.error() << "Number of peaks should not be 0 for SortHKL.\n";
    return;
  }

  std::map<V3D, UniqueReflection> uniqueReflections =
      getUniqueReflections(peaks, outputPeaksWorkspace);

  double rMergeNumerator = 0.0;
  double rPimNumerator = 0.0;
  double intensitySumRValues = 0.0;
  double iOverSigmaSum = 0.0;
  size_t uniqueCount = 0;
  double chiSquared = 0.0;

  peaks.clear();

  for (auto &unique : uniqueReflections) {
    size_t count = unique.second.count();

    /* Since all possible unique reflections are explored
     * there may be 0 observations for some of them.
     * In that case, nothing can be done.*/
    if (count > 0) {
      ++uniqueCount;

      /* Remove any outliers from the statistics calculation,
       * so it needs to be done before the actual statistics are calculated.
       */
      unique.second.removeOutliers();

      // I/sigma is calculated for all reflections, even if there is only one
      // observation.
      std::vector<double> intensities = unique.second.getIntensities();
      std::vector<double> sigmas = unique.second.getSigmas();

      // Accumulate the I/sigma's for current reflection into sum
      iOverSigmaSum += getIOverSigmaSum(sigmas, intensities);

      if (count > 1) {
        // Get mean, standard deviation for intensities
        Statistics intensityStatistics = Kernel::getStatistics(intensities);
        double meanIntensity = intensityStatistics.mean;

        /* This was in the original algorithm, not entirely sure where it is
         * used. It's basically the sum of all relative standard deviations.
         * In a perfect data set with all equivalent reflections exactly
         * equivalent that would be 0. */
        chiSquared += intensityStatistics.standard_deviation / meanIntensity;

        // For both RMerge and RPim sum(|I - <I>|) is required
        double sumOfDeviationsFromMean =
            std::accumulate(intensities.begin(), intensities.end(), 0.0,
                            [meanIntensity](double sum, double intensity) {
                              return sum + fabs(intensity - meanIntensity);
                            });

        // Accumulate into total sum for numerator of RMerge
        rMergeNumerator += sumOfDeviationsFromMean;

        // And Rpim, the sum is weighted by a factor depending on N
        double rPimFactor = sqrt(1.0 / (static_cast<double>(count) - 1.0));
        rPimNumerator += (rPimFactor * sumOfDeviationsFromMean);

        // Collect sum of intensities for R-value calculation
        double reflectionIntensitySum =
            std::accumulate(intensities.begin(), intensities.end(), 0.0);

        intensitySumRValues += reflectionIntensitySum;

        // The original algorithm sets the intensities and sigmas to the mean.
        double sqrtOfMeanSqrSigma = sqrt(getMeanOfSquared(sigmas));
        unique.second.setPeaksIntensityAndSigma(meanIntensity,
                                                sqrtOfMeanSqrSigma);
      }

      unique.second.appendPeaksToVector(peaks);
    }
  }

  double rMerge = rMergeNumerator / intensitySumRValues;
  double rPim = rPimNumerator / intensitySumRValues;

  sortOutputPeaksByHKL(outputPeaksWorkspace);

  double meanIOverSigma = iOverSigmaSum / static_cast<double>(peaks.size());

  std::pair<double, double> lambdaLimits =
      getLambdaLimits(outputPeaksWorkspace);

  std::string name = getProperty("RowName");

  double uniqueReflectionsCount = 0.0;
  if (name.substr(0, 4) != "bank") {
    uniqueReflectionsCount = static_cast<double>(uniqueReflections.size());
  }

  // Generate Statistics table
  const std::string tableName = getProperty("StatisticsTable");
  ITableWorkspace_sptr statisticsTable = getStatisticsTable(tableName);

  if (!statisticsTable) {
    throw std::runtime_error("Problem with ");
  }
  // append to the table workspace
  API::TableRow newrow = statisticsTable->appendRow();

  newrow << name << static_cast<int>(uniqueCount) << lambdaLimits.first
         << lambdaLimits.second
         << static_cast<double>(peaks.size()) / static_cast<double>(uniqueCount)
         << meanIOverSigma << 100.0 * rMerge << 100.0 * rPim
         << 100.0 * static_cast<double>(uniqueCount) /
                static_cast<double>(uniqueReflectionsCount);

  setProperty("OutputWorkspace", outputPeaksWorkspace);
  setProperty("OutputChi2", chiSquared);
  setProperty("StatisticsTable", statisticsTable);
  AnalysisDataService::Instance().addOrReplace(tableName, statisticsTable);
}

/** Rounds the V3D to integer values
* @param hkl the input vector
* @returns The output V3D
*/
V3D SortHKL::round(V3D hkl) {
  V3D hkl1;
  hkl1.setX(round(hkl.X()));
  hkl1.setY(round(hkl.Y()));
  hkl1.setZ(round(hkl.Z()));
  return hkl1;
}

std::map<V3D, UniqueReflection> SortHKL::getPossibleUniqueReflections(
    double dMin, double dMax, const PointGroup_sptr &pointGroup,
    const ReflectionCondition_sptr &centering) const {

  HKLGenerator generator(cell, dMin);
  HKLFilter_const_sptr dFilter =
      boost::make_shared<const HKLFilterDRange>(cell, dMin, dMax);
  HKLFilter_const_sptr centeringFilter =
      boost::make_shared<const HKLFilterCentering>(centering);
  HKLFilter_const_sptr filter = dFilter & centeringFilter;

  // Generate map of UniqueReflection-objects with reflection family as key.
  std::map<V3D, UniqueReflection> uniqueHKLs;
  for (auto hkl : generator) {
    if (filter->isAllowed(hkl)) {
      V3D hklFamily = pointGroup->getReflectionFamily(hkl);
      uniqueHKLs.insert(std::make_pair(hklFamily, UniqueReflection(hklFamily)));
    }
  }

  return uniqueHKLs;
}

/** Rounds a double using 0.5 as the cut off for rounding down
* @param d the input value
* @returns The output value
*/
double SortHKL::round(double d) { return floor(d + 0.5); }

} // namespace Mantid
} // namespace Crystal
