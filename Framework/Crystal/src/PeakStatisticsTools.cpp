#include "MantidCrystal/PeakStatisticsTools.h"

#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"

#include "MantidKernel/Statistics.h"

#include <boost/make_shared.hpp>
#include <numeric>

namespace Mantid {
namespace Crystal {
namespace PeakStatisticsTools {

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/// Returns a vector with the wavelengths of the Peaks stored in this
/// reflection.
std::vector<double> UniqueReflection::getWavelengths() const {
  std::vector<double> wavelengths;
  wavelengths.reserve(m_peaks.size());

  std::transform(
      m_peaks.begin(), m_peaks.end(), std::back_inserter(wavelengths),
      [](const DataObjects::Peak &peak) { return peak.getWavelength(); });

  return wavelengths;
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
UniqueReflection UniqueReflection::removeOutliers(double sigmaCritical,
                                                  bool weightedZ) const {
  if (sigmaCritical <= 0.0) {
    throw std::invalid_argument(
        "Critical sigma value has to be greater than 0.");
  }

  UniqueReflection newReflection(m_hkl);

  if (m_peaks.size() > 2) {
    auto intensities = getIntensities();
    std::vector<double> zScores;
    if (!weightedZ) {
      zScores = Kernel::getZscore(intensities);
    } else {
      auto sigmas = getSigmas();
      zScores = Kernel::getWeightedZscore(intensities, sigmas);
    }

    for (size_t i = 0; i < zScores.size(); ++i) {
      if (zScores[i] <= sigmaCritical) {
        newReflection.addPeak(m_peaks[i]);
      }
    }
  } else {
    for (const auto &peak : m_peaks) {
      newReflection.addPeak(peak);
    }
  }

  return newReflection;
}

/// Sets the intensities and sigmas of all stored peaks to the supplied values.
void UniqueReflection::setPeaksIntensityAndSigma(double intensity,
                                                 double sigma) {
  for (auto &peak : m_peaks) {
    peak.setIntensity(intensity);
    peak.setSigmaIntensity(sigma);
  }
}

/**
 * @brief UniqueReflectionCollection::UniqueReflectionCollection
 *
 * Takes the supplied parameters to calculate theoretically possible
 * unique reflections and stores a UniqueReflection for each of those
 * internally.
 *
 * @param cell :: UnitCell of the sample.
 * @param dLimits :: Resolution limits for the generated reflections.
 * @param pointGroup :: Point group of the sample.
 * @param centering :: Lattice centering.
 */
UniqueReflectionCollection::UniqueReflectionCollection(
    const UnitCell &cell, const std::pair<double, double> &dLimits,
    const PointGroup_sptr &pointGroup,
    const ReflectionCondition_sptr &centering)
    : m_reflections(), m_pointgroup(pointGroup) {
  HKLGenerator generator(cell, dLimits.first);
  auto dFilter = boost::make_shared<const HKLFilterDRange>(cell, dLimits.first,
                                                           dLimits.second);
  auto centeringFilter =
      boost::make_shared<const HKLFilterCentering>(centering);
  auto filter = dFilter & centeringFilter;

  // Generate map of UniqueReflection-objects with reflection family as key.
  for (const auto &hkl : generator) {
    if (filter->isAllowed(hkl)) {
      V3D hklFamily = m_pointgroup->getReflectionFamily(hkl);
      m_reflections.emplace(hklFamily, UniqueReflection(hklFamily));
    }
  }
}

/// Assigns the supplied peaks to the proper UniqueReflection. Peaks for which
/// the reflection family can not be found are ignored.
void UniqueReflectionCollection::addObservations(
    const std::vector<Peak> &peaks) {
  for (auto const &peak : peaks) {
    V3D hkl = peak.getHKL();
    hkl.round();

    auto reflection =
        m_reflections.find(m_pointgroup->getReflectionFamily(hkl));

    if (reflection != m_reflections.end()) {
      (*reflection).second.addPeak(peak);
    }
  }
}

/// Returns a copy of the UniqueReflection with the supplied HKL. Raises an
/// exception if the reflection is not found.
UniqueReflection
UniqueReflectionCollection::getReflection(const V3D &hkl) const {
  return m_reflections.at(m_pointgroup->getReflectionFamily(hkl));
}

/// Total number of unique reflections (theoretically possible).
size_t UniqueReflectionCollection::getUniqueReflectionCount() const {
  return m_reflections.size();
}

/// Number of unique reflections that have more observations than the supplied
/// number (default is 0 - gives number of ).
size_t UniqueReflectionCollection::getObservedUniqueReflectionCount(
    size_t moreThan) const {
  return std::count_if(
      m_reflections.cbegin(), m_reflections.cend(),
      [=](const std::pair<Kernel::V3D, UniqueReflection> &item) {
        return item.second.count() > moreThan;
      });
}

/// List of unobserved unique reflections in resolution range.
std::vector<V3D>
UniqueReflectionCollection::getUnobservedUniqueReflections() const {
  std::vector<V3D> reflections;
  reflections.reserve(m_reflections.size());

  for (const auto &reflection : m_reflections) {
    if (reflection.second.count() == 0) {
      reflections.push_back(reflection.first);
    }
  }

  return reflections;
}

/// Number of observed reflections.
size_t UniqueReflectionCollection::getObservedReflectionCount() const {
  return std::accumulate(
      m_reflections.cbegin(), m_reflections.cend(), size_t(0),
      [](size_t totalReflections,
         const std::pair<Kernel::V3D, UniqueReflection> &item) {
        return totalReflections + item.second.count();
      });
}

/// Returns the internally stored reflection map. May disappear or change if
/// implementation changes.
const std::map<V3D, UniqueReflection> &
UniqueReflectionCollection::getReflections() const {
  return m_reflections;
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
 * @param equivalentIntensities :: Mean or median for statistics of equivalent
 *peaks.
 * @param sigmaCritical :: Number of standard deviations for outliers.
 * @param weightedZ :: True for weighted Zscore
 */
void PeaksStatistics::calculatePeaksStatistics(
    const std::map<V3D, UniqueReflection> &uniqueReflections,
    std::string &equivalentIntensities, double &sigmaCritical,
    bool &weightedZ) {
  double rMergeNumerator = 0.0;
  double rPimNumerator = 0.0;
  double intensitySumRValues = 0.0;
  double iOverSigmaSum = 0.0;

  for (const auto &unique : uniqueReflections) {
    /* Since all possible unique reflections are explored
     * there may be 0 observations for some of them.
     * In that case, nothing can be done.*/
    if (unique.second.count() > 0) {
      ++m_uniqueReflections;

      // Possibly remove outliers.
      auto outliersRemoved =
          unique.second.removeOutliers(sigmaCritical, weightedZ);

      // I/sigma is calculated for all reflections, even if there is only one
      // observation.
      auto intensities = outliersRemoved.getIntensities();
      auto sigmas = outliersRemoved.getSigmas();

      // Accumulate the I/sigma's for current reflection into sum
      iOverSigmaSum += getIOverSigmaSum(sigmas, intensities);

      if (outliersRemoved.count() > 1) {
        // Get mean, standard deviation for intensities
        auto intensityStatistics = Kernel::getStatistics(
            intensities, StatOptions::Mean | StatOptions::UncorrectedStdDev |
                             StatOptions::Median);

        double meanIntensity = intensityStatistics.mean;
        if (equivalentIntensities == "Median")
          meanIntensity = intensityStatistics.median;

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
            sqrt(1.0 / (static_cast<double>(outliersRemoved.count()) - 1.0));
        rPimNumerator += (rPimFactor * sumOfDeviationsFromMean);

        // Collect sum of intensities for R-value calculation
        intensitySumRValues +=
            std::accumulate(intensities.begin(), intensities.end(), 0.0);

        // The original algorithm sets the intensities and sigmas to the mean.
        double sqrtOfMeanSqrSigma = getRMS(sigmas);
        outliersRemoved.setPeaksIntensityAndSigma(meanIntensity,
                                                  sqrtOfMeanSqrSigma);
      }

      const std::vector<Peak> &reflectionPeaks = outliersRemoved.getPeaks();
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

    auto dspacingLimits = getDSpacingLimits(m_peaks);
    m_dspacingMin = dspacingLimits.first;
    m_dspacingMax = dspacingLimits.second;
  }
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
PeaksStatistics::getDSpacingLimits(const std::vector<Peak> &peaks) const {
  if (peaks.empty()) {
    return std::make_pair(0.0, 0.0);
  }

  auto dspacingLimitIterators = std::minmax_element(
      peaks.begin(), peaks.end(), [](const Peak &lhs, const Peak &rhs) {
        return lhs.getDSpacing() < rhs.getDSpacing();
      });

  return std::make_pair((*(dspacingLimitIterators.first)).getDSpacing(),
                        (*(dspacingLimitIterators.second)).getDSpacing());
}

} // namespace PeakStatisticsTools
} // namespace Crystal
} // namespace Mantid
