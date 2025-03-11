// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiPeakSearch.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V2D.h"

#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <algorithm>
#include <list>
#include <numeric>
#include <queue>
#include <utility>

#include "boost/math/distributions.hpp"

#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidKernel/MultiThreaded.h"

namespace Mantid::Poldi {

DECLARE_ALGORITHM(PoldiPeakSearch)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using HistogramData::HistogramY;

PoldiPeakSearch::PoldiPeakSearch()
    : API::Algorithm(), m_minimumDistance(0), m_doubleMinimumDistance(0), m_minimumPeakHeight(0.0),
      m_maximumPeakNumber(0), m_peaks(new PoldiPeakCollection()) {}

/** Sums the counts of neighboring d-values
 *
 * This method takes a vector of counts y with N elements and produces a new
 *vector y'
 * with N - 2 elements, such that y'[i] = y[i-1] + y[i] + y[i+1].
 *
 * @param correlationCounts :: Vector with correlation counts.
 * @return Vector with sum of neighboring correlation counts.
 */
MantidVec PoldiPeakSearch::getNeighborSums(const HistogramY &correlationCounts) const {
  /* Since the first and last element in a list don't have two neighbors, they
   *are excluded from the calculation
   * and the result vector's size is reduced by two. Also, the algorithm does
   *not work on vectors with fewer
   * than three elements.
   **/
  size_t counts = correlationCounts.size();

  if (counts < 3) {
    throw std::runtime_error("A vector with less than three elements can not be processed.");
  }

  size_t validCounts = counts - 2;

  size_t n = 1;
  std::vector<size_t> validIndices(validCounts);
  for (size_t i = 0; i < validCounts; ++i) {
    validIndices[i] = n++;
  }

  MantidVec summedNeighborCounts;
  summedNeighborCounts.reserve(validCounts);

  for (std::vector<size_t>::const_iterator index = validIndices.begin(); index != validIndices.end(); ++index) {
    summedNeighborCounts.emplace_back(correlationCounts[(*index) - 1] + correlationCounts[*index] +
                                      correlationCounts[(*index) + 1]);
  }

  return summedNeighborCounts;
}

/** Detects peaks in supplied vector
 *
 * This method returns a list of iterators that point to peak positions in the
 *range
 * defined by the arguments (start and end iterators). While the actual peak
 *search is
 * performed by PoldiPeakSearch::findPeaksRecursive, this method does some book
 *keeping
 * such as configuring borders of the vector and after searching, reducing the
 *peak list
 * according to the limit specified by MaximumPeakNumber.
 *
 * @param begin :: Iterator that points to the start of the vector to be
 *searched.
 * @param end :: End iterator.
 * @return List of iterators which indicate peak positions with respect to the
 *input range.
 */
std::list<MantidVec::const_iterator> PoldiPeakSearch::findPeaks(MantidVec::const_iterator begin,
                                                                MantidVec::const_iterator end) {

  std::list<MantidVec::const_iterator> rawPeaks = findPeaksRecursive(begin, end);

  /* The recursive algorithms potentially finds maxima that are not peaks,
   * so the list is truncated to the maximum desired peak number (N), where
   * only the N strongest peaks are kept.
   */
  rawPeaks.sort(&PoldiPeakSearch::vectorElementGreaterThan);

  size_t usedPeakCount = std::min(m_maximumPeakNumber, static_cast<int>(rawPeaks.size()));
  auto lastPeakIter = std::next(rawPeaks.begin(), usedPeakCount);

  return std::list<MantidVec::const_iterator>(rawPeaks.begin(), lastPeakIter);
}

/** Actual recursive peak search method
 *
 * The maximum of the vector specified by the two iterators supplied as
 *parameters is designated
 * a peak in the given range and appended to a list. Then, the method is
 *executed on the two sub-ranges
 * of the vector [begin, max - m] and [max + m, end], where m is the user
 *supplied value of MinimumPeakSeparation.
 * The results of these two sub-searches are merged into the list, which is
 *then returned.
 *
 * @param begin :: Starting iterator of currently searched vector.
 * @param end :: End iterator.
 * @return List of iterators which designate peaks in this range and all
 *sub-ranges.
 */
std::list<MantidVec::const_iterator> PoldiPeakSearch::findPeaksRecursive(MantidVec::const_iterator begin,
                                                                         MantidVec::const_iterator end) const {
  // find the maximum intensity in the range (begin - end)...
  auto maxInRange = std::max_element(begin, end);

  std::list<MantidVec::const_iterator> peaks;
  peaks.emplace_back(maxInRange);

  // ...and perform same search on sub-list left of maximum...
  if (std::distance(begin, maxInRange) > m_minimumDistance) {
    std::list<MantidVec::const_iterator> leftBranchPeaks = findPeaksRecursive(begin, maxInRange - m_minimumDistance);
    peaks.insert(peaks.end(), leftBranchPeaks.begin(), leftBranchPeaks.end());
  }

  // ...and right of maximum
  if (std::distance(maxInRange + 1, end) > m_minimumDistance) {
    std::list<MantidVec::const_iterator> rightBranchPeaks = findPeaksRecursive(maxInRange + 1 + m_minimumDistance, end);
    peaks.insert(peaks.end(), rightBranchPeaks.begin(), rightBranchPeaks.end());
  }

  return peaks;
}

/** Maps peak position iterators from one vector to another
 *
 * This method is required because the actual peak search is better performed
 *in the summed neighbor data (see PoldiPeakSearch::getNeighborSums)
 * and the detected peak positions have to be mapped to the original
 *correlation spectrum (shifted by 1).
 *
 * @param peakPositions :: List with peak position iterators.
 * @param baseDataStart :: Starting iterator of the vector in which the peak
 *search was performed.
 * @param originalDataStart :: Starting iterator of the vector with the
 *original correlation data.
 * @returns List with peak positions mapped to the original correlation count
 *data.
 */
std::list<MantidVec::const_iterator>
PoldiPeakSearch::mapPeakPositionsToCorrelationData(const std::list<MantidVec::const_iterator> &peakPositions,
                                                   MantidVec::const_iterator baseDataStart,
                                                   MantidVec::const_iterator originalDataStart) const {
  std::list<MantidVec::const_iterator> transformedIndices;

  for (std::list<MantidVec::const_iterator>::const_iterator peakPosition = peakPositions.begin();
       peakPosition != peakPositions.end(); ++peakPosition) {
    transformedIndices.emplace_back(originalDataStart + std::distance(baseDataStart, *peakPosition) + 1);
  }

  return transformedIndices;
}

/// Converts the value-parameter to d-spacing. Assumes unit to be Q if empty.
double PoldiPeakSearch::getTransformedCenter(double value, const Unit_sptr &unit) const {
  if (std::dynamic_pointer_cast<Units::dSpacing>(unit)) {
    return value;
  }

  // This is required to preserve default behavior which assumes Q.
  Unit_sptr transformUnit = unit;

  if (!unit || std::dynamic_pointer_cast<Units::Empty>(unit)) {
    transformUnit = UnitFactory::Instance().create("MomentumTransfer");
  }

  // Transform value to d-spacing.
  Unit_sptr dUnit = UnitFactory::Instance().create("dSpacing");
  return UnitConversion::run((*transformUnit), (*dUnit), value, 0, DeltaEMode::Elastic);
}

/** Creates PoldiPeak-objects from peak position iterators
 *
 * In this method, PoldiPeak objects are created from the raw peak position
 *data and the original x-data. Estimates for peak height and FWHM
 * provided along with the position.
 *
 * @param baseListStart :: Starting iterator of the vector which the peak
 *positions refer to.
 * @param peakPositions :: List with peakPositions.
 * @param xData :: Vector with x-values of the correlation spectrum.
 * @return Vector with PoldiPeak objects constructed from the raw peak position
 *data.
 */
std::vector<PoldiPeak_sptr> PoldiPeakSearch::getPeaks(const MantidVec::const_iterator &baseListStart,
                                                      const MantidVec::const_iterator &baseListEnd,
                                                      std::list<MantidVec::const_iterator> peakPositions,
                                                      const MantidVec &xData, const Unit_sptr &unit) const {
  std::vector<PoldiPeak_sptr> peakData;
  peakData.reserve(peakPositions.size());

  for (std::list<MantidVec::const_iterator>::const_iterator peak = peakPositions.begin(); peak != peakPositions.end();
       ++peak) {
    size_t index = std::distance(baseListStart, *peak);

    double xDataD = getTransformedCenter(xData[index], unit);

    double fwhmEstimate = getFWHMEstimate(baseListStart, baseListEnd, *peak, xData);
    UncertainValue fwhm(fwhmEstimate / xData[index]);

    PoldiPeak_sptr newPeak = PoldiPeak::create(MillerIndices(), UncertainValue(xDataD), UncertainValue(**peak), fwhm);
    peakData.emplace_back(newPeak);
  }

  return peakData;
}

/** Generates an estimate for FWHM for each peak
 *
 * To make it easier for fitting routines, rough FWHM estimates are calculated.
 *
 * @param baseListStart :: Starting iterator of the vector which the peak
 *positions refer to.
 * @param peakPosition :: Peak position iterator.
 * @param xData :: Vector with x-values of the correlation spectrum.
 * @return Estimation of FWHM
 */
double PoldiPeakSearch::getFWHMEstimate(const MantidVec::const_iterator &baseListStart,
                                        const MantidVec::const_iterator &baseListEnd,
                                        MantidVec::const_iterator peakPosition, const MantidVec &xData) const {
  size_t peakPositionIndex = std::distance(baseListStart, peakPosition);
  double halfPeakIntensity = *peakPosition / 2.0;

  /* - walk to first point i with intensity < maximum/2
   * - average positions i-1 and i as guess for position of fwhm
   * - return difference to peak position * 2
   */
  auto nextIntensity = peakPosition;
  while (nextIntensity != baseListEnd && (*nextIntensity > halfPeakIntensity)) {
    nextIntensity += 1;
  }

  if (nextIntensity != baseListEnd) {
    size_t fwhmIndex = std::distance(baseListStart, nextIntensity);
    double hmXGuess = (xData[fwhmIndex - 1] + xData[fwhmIndex]) / 2.0;

    return (hmXGuess - xData[peakPositionIndex]) * 2.0;
  }

  return 0.002;
}

/** Sets error of workspace to specified value
 *
 * Since an estimation of the error is calculated from background counts, this
 *value is assigned to the workspace via this method.
 *
 * @param correlationWorkspace :: Workspace containing the correlation spectrum
 *on which the peak search was performed.
 * @param error :: Error that is set on the workspace.
 */
void PoldiPeakSearch::setErrorsOnWorkspace(const Workspace2D_sptr &correlationWorkspace, double error) const {
  MantidVec &errors = correlationWorkspace->dataE(0);

  std::fill(errors.begin(), errors.end(), error);
}

/** Retrieves a vector with all counts that belong to the background
 *
 * In this method, a vector is assembled which contains all count data that is
 *considered to be background.
 * Whether a point is considered background depends on its distance to the
 *given peak positions.
 *
 * @param peakPositions :: Peak positions.
 * @param correlationCounts :: Vector with the complete correlation spectrum.
 * @return Vector only with counts that belong to the background.
 */
MantidVec PoldiPeakSearch::getBackground(const std::list<MantidVec::const_iterator> &peakPositions,
                                         const MantidVec &correlationCounts) const {
  size_t backgroundPoints = getNumberOfBackgroundPoints(peakPositions, correlationCounts);

  MantidVec background;
  background.reserve(backgroundPoints);

  for (auto point = correlationCounts.cbegin() + 1; point != correlationCounts.cend() - 1; ++point) {
    if (distanceToPeaksGreaterThanMinimum(peakPositions, point)) {
      background.emplace_back(*point);
    }
  }

  return background;
}

/** Computes a background estimation with an error
 *
 * This method computes an estimate of the average background along with its
 *deviation. Since the background does not
 * follow a normal distribution and may contain outliers, instead of computing
 *the average and standard deviation,
 * the median is used as location estimator and Sn is used as scale estimator.
 *For details regarding the latter
 * refer to PoldiPeakSearch::getSn.
 *
 * @param peakPositions :: Peak positions.
 * @param correlationCounts :: Data from which the peak positions were
 *extracted.
 * @return Background estimation with error.
 */
UncertainValue PoldiPeakSearch::getBackgroundWithSigma(const std::list<MantidVec::const_iterator> &peakPositions,
                                                       const MantidVec &correlationCounts) const {
  MantidVec background = getBackground(peakPositions, correlationCounts);

  /* Instead of using Mean and Standard deviation, which are appropriate
   * for data originating from a normal distribution (which is not the case
   * for background of POLDI correlation spectra), the more robust measures
   * Median and Sn are used.
   */
  std::sort(background.begin(), background.end());
  double meanBackground = getMedianFromSortedVector(background.begin(), background.end());
  double sigmaBackground = getSn(background.begin(), background.end());

  return UncertainValue(meanBackground, sigmaBackground);
}

/** Checks if the distance of a given point is smaller than the minimum peak
 *separation
 *
 * The method is required for background extraction.
 *
 * @param peakPositions :: Peak positions.
 * @param point :: Point in the spectrum to check.
 * @return Boolean value, true if point is not closer than
 *MinimumPeakSeparation to any peakPosition, false otherwise.
 */
bool PoldiPeakSearch::distanceToPeaksGreaterThanMinimum(std::list<MantidVec::const_iterator> peakPositions,
                                                        MantidVec::const_iterator point) const {
  return std::none_of(peakPositions.cbegin(), peakPositions.cend(), [&](const auto peakPosition) {
    return std::abs(std::distance(point, peakPosition)) <= m_minimumDistance;
  });
}

/** Returns the number of background points
 *
 * Given a list of peaks and a vector with spectrum count data, this method
 *returns the number of background points
 * contained in this spectrum. It is used for extraction of the background in
 *order to allocate the correct vector
 * size before filling it.
 *
 * @param peakPositions :: Peak positions.
 * @param correlationCounts :: Data vector the peak positions refer to.
 * @return Number of background points.
 */
size_t PoldiPeakSearch::getNumberOfBackgroundPoints(const std::list<MantidVec::const_iterator> &peakPositions,
                                                    const MantidVec &correlationCounts) const {
  /* subtracting 2, to match original algorithm, where
   * the first and the last point of the spectrum are not
   * considered in this calculation.
   */
  size_t totalDataPoints = correlationCounts.size() - 2;
  size_t occupiedByPeaks = peakPositions.size() * (m_doubleMinimumDistance + 1);

  if (occupiedByPeaks > totalDataPoints) {
    throw(std::runtime_error("More data points occupied by peaks than existing "
                             "data points - not possible."));
  }

  return totalDataPoints - occupiedByPeaks;
}

/** Returns median of a sorted vector
 *
 * @param begin :: Beginning of vector.
 * @param end :: End of vector.
 * @return Median of supplied data.
 */
double PoldiPeakSearch::getMedianFromSortedVector(MantidVec::const_iterator begin,
                                                  MantidVec::const_iterator end) const {
  size_t count = std::distance(begin, end);

  if (count % 2 == 0) {
    return 0.5 * (*(begin + (count / 2) - 1) + *(begin + (count / 2)));
  } else {
    return *(begin + (count + 1) / 2 - 1);
  }
}

/** Calculates Sn as estimator of scale for given vector
 *
 * This method implements a naive calculation of Sn, as defined by Rousseeuw
 *and Croux (http://dx.doi.org/10.2307%2F2291267).
 * In contrast to standard deviation, this is more robust towards outliers.
 *
 * @param begin :: Beginning of vector.
 * @param end :: End of vector.
 * @return Sn of supplied data.
 */
double PoldiPeakSearch::getSn(MantidVec::const_iterator begin, MantidVec::const_iterator end) const {
  size_t numberOfPoints = std::distance(begin, end);
  MantidVec absoluteDifferenceMedians(numberOfPoints);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(numberOfPoints); ++i) {
    double currentValue = *(begin + i);
    MantidVec temp;
    temp.reserve(numberOfPoints - 1);
    for (int j = 0; j < static_cast<int>(numberOfPoints); ++j) {
      if (j != i) {
        temp.emplace_back(fabs(*(begin + j) - currentValue));
      }
    }
    std::sort(temp.begin(), temp.end());

    absoluteDifferenceMedians[i] = getMedianFromSortedVector(temp.begin(), temp.end());
  }

  std::sort(absoluteDifferenceMedians.begin(), absoluteDifferenceMedians.end());

  return 1.1926 * getMedianFromSortedVector(absoluteDifferenceMedians.begin(), absoluteDifferenceMedians.end());
}

/** Returns the minimum height a peak should have.
 *
 * This method is used when no user estimate of minimum required peak height is
 *given. It is set to background
 * plus 3 times Sn.
 *
 * @param backgroundWithSigma :: Background with error estimate.
 * @return Minimum peak height.
 */
double PoldiPeakSearch::minimumPeakHeightFromBackground(UncertainValue backgroundWithSigma) const {
  return 3.0 * backgroundWithSigma.error() + backgroundWithSigma.value();
}

void PoldiPeakSearch::setMinimumDistance(int newMinimumDistance) {
  if (newMinimumDistance <= 0) {
    throw std::runtime_error("The distance between peaks has to be larger than 0.");
  }

  m_minimumDistance = newMinimumDistance;
  m_doubleMinimumDistance = 2 * m_minimumDistance;
}

void PoldiPeakSearch::setMinimumPeakHeight(double newMinimumPeakHeight) { m_minimumPeakHeight = newMinimumPeakHeight; }

void PoldiPeakSearch::setMaximumPeakNumber(int newMaximumPeakNumber) { m_maximumPeakNumber = newMaximumPeakNumber; }

bool PoldiPeakSearch::vectorElementGreaterThan(MantidVec::const_iterator first, MantidVec::const_iterator second) {
  return *first > *second;
}

bool PoldiPeakSearch::isLessThanMinimum(const PoldiPeak_sptr &peak) {
  return peak->intensity().value() <= m_minimumPeakHeight;
}

void PoldiPeakSearch::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("InputWorkspace", "", Direction::InOut),
                  "Workspace containing a POLDI auto-correlation spectrum.");

  std::shared_ptr<BoundedValidator<int>> minPeakSeparationValidator = std::make_shared<BoundedValidator<int>>();
  minPeakSeparationValidator->setLower(1);
  declareProperty("MinimumPeakSeparation", 15, minPeakSeparationValidator,
                  "Minimum number of points in the spectrum by which two peaks "
                  "have to be separated.",
                  Direction::Input);

  std::shared_ptr<BoundedValidator<int>> maxPeakNumberValidator = std::make_shared<BoundedValidator<int>>();
  maxPeakNumberValidator->setLower(1);
  declareProperty("MaximumPeakNumber", 24, maxPeakNumberValidator, "Maximum number of peaks to be detected.",
                  Direction::Input);

  declareProperty("MinimumPeakHeight", 0.0, "Minimum peak height.", Direction::Input);

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Workspace containing detected peaks.");
}

void PoldiPeakSearch::exec() {
  g_log.information() << "PoldiPeakSearch:\n";

  Workspace2D_sptr correlationWorkspace = getProperty("InputWorkspace");
  auto &correlationQValues = correlationWorkspace->x(0);
  auto &correlatedCounts = correlationWorkspace->y(0);
  g_log.information() << "   Auto-correlation data read.\n";

  Unit_sptr xUnit = correlationWorkspace->getAxis(0)->unit();

  if (xUnit->caption().empty()) {
    g_log.information() << "   Workspace does not have unit, defaulting to MomentumTransfer.\n";

    xUnit = UnitFactory::Instance().create("MomentumTransfer");
  } else {
    g_log.information() << "   Unit of workspace is " << xUnit->caption() << ".\n";
  }

  setMinimumDistance(getProperty("MinimumPeakSeparation"));
  setMinimumPeakHeight(getProperty("MinimumPeakHeight"));
  setMaximumPeakNumber(getProperty("MaximumPeakNumber"));

  if (m_doubleMinimumDistance > static_cast<int>(correlatedCounts.size())) {
    throw(std::runtime_error("MinimumPeakSeparation is smaller than number of "
                             "spectrum points - no peaks possible."));
  }

  g_log.information() << "   Parameters set.\n";

  MantidVec summedNeighborCounts = getNeighborSums(correlatedCounts.rawData());
  g_log.information() << "   Neighboring counts summed, contains " << summedNeighborCounts.size() << " data points.\n";

  std::list<MantidVec::const_iterator> peakPositionsSummed =
      findPeaks(summedNeighborCounts.cbegin(), summedNeighborCounts.cend());
  g_log.information() << "   Peaks detected in summed spectrum: " << peakPositionsSummed.size() << '\n';

  /* This step is required because peaks are actually searched in the
   * "sum-of-neighbors"-spectrum.
   * The mapping removes the offset from the peak position which results from
   * different beginning
   * of this vector compared to the original correlation counts.
   */
  std::list<MantidVec::const_iterator> peakPositionsCorrelation =
      mapPeakPositionsToCorrelationData(peakPositionsSummed, summedNeighborCounts.begin(), correlatedCounts.begin());
  g_log.information() << "   Peak positions transformed to original spectrum.\n";

  /* Since intensities are required for filtering, they are extracted from the
   * original count data,
   * along with the Q-values.
   */
  std::vector<PoldiPeak_sptr> peakCoordinates = getPeaks(correlatedCounts.cbegin(), correlatedCounts.cend(),
                                                         peakPositionsCorrelation, correlationQValues.rawData(), xUnit);
  g_log.information() << "   Extracted peak positions in Q and intensity guesses.\n";

  UncertainValue backgroundWithSigma = getBackgroundWithSigma(peakPositionsCorrelation, correlatedCounts.rawData());
  g_log.information() << "   Calculated average background and deviation: "
                      << UncertainValueIO::toString(backgroundWithSigma) << '\n';

  if ((*getProperty("MinimumPeakHeight")).isDefault()) {
    setMinimumPeakHeight(minimumPeakHeightFromBackground(backgroundWithSigma));
  }

  std::vector<PoldiPeak_sptr> intensityFilteredPeaks(peakCoordinates.size());
  using namespace std::placeholders;
  auto newEnd = std::remove_copy_if(peakCoordinates.begin(), peakCoordinates.end(), intensityFilteredPeaks.begin(),
                                    std::bind(&PoldiPeakSearch::isLessThanMinimum, this, _1));
  intensityFilteredPeaks.resize(std::distance(intensityFilteredPeaks.begin(), newEnd));

  g_log.information() << "   Peaks above minimum intensity (" << m_minimumPeakHeight
                      << "): " << intensityFilteredPeaks.size() << '\n';

  std::sort(intensityFilteredPeaks.begin(), intensityFilteredPeaks.end(),
            std::bind(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::intensity));

  for (std::vector<PoldiPeak_sptr>::const_iterator peak = intensityFilteredPeaks.begin();
       peak != intensityFilteredPeaks.end(); ++peak) {
    m_peaks->addPeak(*peak);
  }

  /* The derived background error is set as error in the workspace containing
   * correlation data, so it may be used as weights for peak fitting later on.
   */
  setErrorsOnWorkspace(correlationWorkspace, backgroundWithSigma.error());

  setProperty("OutputWorkspace", m_peaks->asTableWorkspace());
}

} // namespace Mantid::Poldi
