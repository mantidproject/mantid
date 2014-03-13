/*WIKI*
PoldiPeakSearch is a peak-finding routine for POLDI auto-correlation data. The algorithm is implemented according
to the original data analysis software and their results match closely.

The algorithm performs the following steps:
# Map each point of the spectrum <math>y</math>, except the first and the last to the sum of its value and its neighbor's values:
    <math>y'_i = y_{i-1} + y_{i} + y_{i+1}</math>
  The new spectrum <math>y'</math> contains <math>n-2</math> points when <math>y</math> contains <math>n</math>.
# Identify peak positions in <math>y'</math>, which is done with a recursive algorithm, consisting of these steps:
## Find the position of the maximum, <math>i_{max}</math> in the list, store in peak-list.
## Split the list in two parts, <math>[i_{0} + \Delta, i_{max} - \Delta)</math> and <math>(i_{max} + \Delta, i_{n} - \Delta]</math>,
where <math>\Delta</math> is the mininum number of data points between two peaks.
## If ranges are valid, perform algorithm on each of the sublists, append returned lists to peak-list.
## Return peak-list.
# Sort list by value in descending order, keep the first <math>N_{max}</math> items of the list.
# Map peak positions from <math>y'</math> back to <math>y</math>
# Perform background and fluctuation estimation:
## Extract all points from <math>y</math> (except the first and the last) that are further than <math>\Delta</math> elements away from any peak position
## Calculate average background, <math>\bar{b}</math>, from these points.
## Calculate average absolute difference, <math>\bar{s}</math>: <math>|y_{i} - y_{i+1}|</math>
# Estimate peak intensity as <math>y'_{i} / 3</math>
# If a minimum peak height is set, discard all peaks that are smaller than this, if not, discard all peaks that are lower than <math>2.75\cdot\bar{s} + \bar{b}</math>

The peaks are stored in a new workspace.
*WIKI*/

#include "MantidSINQ/PoldiPeakSearch.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/V2D.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "boost/bind.hpp"
#include <list>
#include <algorithm>
#include <numeric>

namespace Mantid
{
namespace Poldi
{

DECLARE_ALGORITHM(PoldiPeakSearch)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

PoldiPeakSearch::PoldiPeakSearch() :
    API::Algorithm(),
    m_minimumDistance(0),
    m_doubleMinimumDistance(0),
    m_minimumPeakHeight(0.0),
    m_maximumPeakNumber(0),
    m_recursionAbsoluteBegin(0),
    m_recursionAbsoluteEnd(0)
{
}

void PoldiPeakSearch::initDocs()
{
    setWikiSummary("This algorithm finds the peaks in a POLDI auto-correlation spectrum.");
}

MantidVec PoldiPeakSearch::getNeighborSums(MantidVec correlationCounts)
{
    /* Since the first and last element in a list don't have two neighbors, they are excluded from the calculation
     * and the result vector's size is reduced by two. Also, the algorithm does not work on vectors with fewer
     * than three elements.
     **/
    size_t counts = correlationCounts.size();

    if(counts < 3) {
        throw std::runtime_error("A vector with less than three elements can not be processed.");
    }

    size_t validCounts = counts - 2;

    size_t n = 1;
    std::vector<size_t> validIndices(validCounts);
    std::generate(validIndices.begin(), validIndices.end(), [&n] () { return n++; });

    MantidVec summedNeighborCounts;
    summedNeighborCounts.reserve(validCounts);

    for(std::vector<size_t>::const_iterator index = validIndices.begin(); index != validIndices.end(); ++index) {
        summedNeighborCounts.push_back(correlationCounts[(*index) - 1] + correlationCounts[*index] + correlationCounts[(*index) + 1]);
    }

    return summedNeighborCounts;
}

std::list<MantidVec::iterator> PoldiPeakSearch::findPeaks(MantidVec::iterator begin, MantidVec::iterator end) {
    // These borders need to be known for handling the edges correctly in the recursion
    setRecursionAbsoluteBorders(begin, end);

    std::list<MantidVec::iterator> rawPeaks = findPeaksRecursive(begin, end);

    /* The recursive algorithms potentially finds maxima that are not peaks,
     * so the list is truncated to the maximum desired peak number (N), where
     * only the N strongest peaks are kept.
     */
    rawPeaks.sort(&PoldiPeakSearch::vectorElementGreaterThan);//[](MantidVec::iterator first, MantidVec::iterator second) { return (*first) > (*second); } );

    std::list<MantidVec::iterator>::iterator rawPeaksLimit = std::next(rawPeaks.begin(), std::min(m_maximumPeakNumber, static_cast<int>(rawPeaks.size())));
    std::list<MantidVec::iterator> truncatedPeaks(rawPeaks.begin(), rawPeaksLimit);

    return truncatedPeaks;
}

std::list<MantidVec::iterator> PoldiPeakSearch::findPeaksRecursive(MantidVec::iterator begin, MantidVec::iterator end)
{
    // find the maximum intensity in the range (begin - end)...
    MantidVec::iterator maxInRange = std::max_element(begin, end);

    std::list<MantidVec::iterator> peaks;
    peaks.push_back(maxInRange);

    // ...and perform same search on sub-list left of maximum...
    MantidVec::iterator leftBegin = getLeftRangeBegin(begin);
    if(std::distance(leftBegin, maxInRange) > m_minimumDistance) {
        peaks.merge(findPeaksRecursive(leftBegin, maxInRange - m_minimumDistance));
    }

    // ...and right of maximum
    MantidVec::iterator rightEnd = getRightRangeEnd(end);
    if(std::distance(maxInRange + 1, rightEnd) > m_minimumDistance) {
        peaks.merge(findPeaksRecursive(maxInRange + 1 + m_minimumDistance, rightEnd));
    }

    return peaks;
}

MantidVec::iterator PoldiPeakSearch::getLeftRangeBegin(MantidVec::iterator begin)
{
    /* The edges of the searched range require special treatment. Without this sanitation,
     * each recursion step that includes the leftmost sublist would chop off m_minimumDistance
     * elements from the beginning, so the index is compared to the range's absolute start.
     *
     * Exactly the same considerations are valid for the rightmost sublist.
     */
    if(begin != m_recursionAbsoluteBegin) {
        return begin + m_minimumDistance;
    }

    return begin;
}

MantidVec::iterator PoldiPeakSearch::getRightRangeEnd(MantidVec::iterator end)
{
    if(end != m_recursionAbsoluteEnd) {
        return end - m_minimumDistance;
    }

    return end;
}

std::list<MantidVec::iterator> PoldiPeakSearch::mapPeakPositionsToCorrelationData(std::list<MantidVec::iterator> peakPositions, MantidVec::iterator baseDataStart, MantidVec::iterator originalDataStart)
{
    std::list<MantidVec::iterator> transformedIndices;

    for(std::list<MantidVec::iterator>::const_iterator peakPosition = peakPositions.begin(); peakPosition != peakPositions.end(); ++peakPosition) {
        transformedIndices.push_back(originalDataStart + std::distance(baseDataStart, *peakPosition) + 1);
    }

    return transformedIndices;
}

std::vector<V2D> PoldiPeakSearch::getPeakCoordinates(MantidVec::iterator baseListStart, std::list<MantidVec::iterator> peakPositions, MantidVec xData)
{
    std::vector<V2D> peakData;
    peakData.reserve(peakPositions.size());

    for(std::list<MantidVec::iterator>::const_iterator peak = peakPositions.begin();
        peak != peakPositions.end();
        ++peak)
    {
        size_t index = std::distance(baseListStart, *peak) + 1;
        peakData.push_back(V2D(xData[index], **peak / 3.0));
    }

    return peakData;
}

void PoldiPeakSearch::setErrorsOnWorkspace(Workspace2D_sptr correlationWorkspace, double error)
{
    MantidVec &errors = correlationWorkspace->dataE(0);

    std::fill(errors.begin(), errors.end(), error);
}

std::pair<double, double> PoldiPeakSearch::getBackgroundWithSigma(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts)
{
    size_t backgroundPoints = getNumberOfBackgroundPoints(peakPositions, correlationCounts);

    MantidVec sigma;
    sigma.reserve(backgroundPoints);

    MantidVec background;
    background.reserve(backgroundPoints);

    for(MantidVec::iterator point = correlationCounts.begin() + 1;
        point != correlationCounts.end() - 1;
        ++point)
    {
        if(distanceToPeaksGreaterThanMinimum(peakPositions, point)) {
            background.push_back(*point);
            sigma.push_back(std::abs(*(point) - *(point + 1)));
        }
    }

    double sumBackground = std::accumulate(background.begin(), background.end(), 0.0);
    double sumSigma = std::accumulate(sigma.begin(), sigma.end(), 0.0);

    return std::make_pair(sumBackground / static_cast<double>(background.size()), sumSigma / static_cast<double>(sigma.size()));
}

bool PoldiPeakSearch::distanceToPeaksGreaterThanMinimum(std::list<MantidVec::iterator> peakPositions, MantidVec::iterator point)
{
    for(std::list<MantidVec::iterator>::const_iterator peakPosition = peakPositions.begin(); peakPosition != peakPositions.end(); ++peakPosition) {
        if(std::abs(std::distance(point, *peakPosition)) <= m_minimumDistance) {
            return false;
        }
    }

    return true;
}

size_t PoldiPeakSearch::getNumberOfBackgroundPoints(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts)
{
    /* subtracting 2, to match original algorithm, where
     * the first and the last point of the spectrum are not
     * considered in this calculation.
     */
    size_t totalDataPoints = correlationCounts.size() - 2;
    size_t occupiedByPeaks = peakPositions.size() * (m_doubleMinimumDistance + 1);

    if(occupiedByPeaks > totalDataPoints) {
        throw(std::runtime_error("More data points occupied by peaks than existing data points - not possible."));
    }

    return totalDataPoints - occupiedByPeaks;
}

double PoldiPeakSearch::minimumPeakHeightFromBackground(std::pair<double, double> backgroundWithSigma)
{
    return 2.75 * backgroundWithSigma.second + backgroundWithSigma.first;
}

void PoldiPeakSearch::setMinimumDistance(int newMinimumDistance)
{
    if(newMinimumDistance <= 0) {
        throw std::runtime_error("The distance between peaks has to be larger than 0.");
    }

    m_minimumDistance = newMinimumDistance;
    m_doubleMinimumDistance = 2 * m_minimumDistance;
}

void PoldiPeakSearch::setMinimumPeakHeight(double newMinimumPeakHeight)
{
    m_minimumPeakHeight = newMinimumPeakHeight;
}

void PoldiPeakSearch::setMaximumPeakNumber(int newMaximumPeakNumber)
{
    m_maximumPeakNumber = newMaximumPeakNumber;
}

void PoldiPeakSearch::setRecursionAbsoluteBorders(MantidVec::iterator begin, MantidVec::iterator end)
{
    m_recursionAbsoluteBegin = begin;
    m_recursionAbsoluteEnd = end;
}

bool PoldiPeakSearch::vectorElementGreaterThan(MantidVec::iterator first, MantidVec::iterator second)
{
    return *first > *second;
}

bool PoldiPeakSearch::yGreaterThan(const V2D first, const V2D second)
{
    return first.Y() > second.Y();
}

bool PoldiPeakSearch::isLessThanMinimum(V2D peakCoordinate)
{
    return peakCoordinate.Y() <= m_minimumPeakHeight;
}

void PoldiPeakSearch::init()
{
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace", "", Direction::InOut),
                    "Workspace containing a POLDI auto-correlation spectrum.");

    boost::shared_ptr<BoundedValidator<int> > minPeakSeparationValidator = boost::make_shared<BoundedValidator<int> >();
    minPeakSeparationValidator->setLower(1);
    declareProperty("MinimumPeakSeparation", 15, minPeakSeparationValidator, "Minimum number of points in the spectrum by which two peaks have to be separated.", Direction::Input);

    boost::shared_ptr<BoundedValidator<int> > maxPeakNumberValidator = boost::make_shared<BoundedValidator<int> >();
    maxPeakNumberValidator->setLower(1);
    declareProperty("MaximumPeakNumber", 24, maxPeakNumberValidator, "Maximum number of peaks to be detected.", Direction::Input);

    declareProperty("MinimumPeakHeight", 0.0, "Minimum peak height.", Direction::Input);

    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace", "", Direction::Output),
                    "Workspace containing detected peaks.");
}

void PoldiPeakSearch::exec()
{
    g_log.information() << "PoldiPeakSearch:" << std::endl;

    Workspace2D_sptr correlationWorkspace = getProperty("InputWorkspace");
    MantidVec correlationQValues = correlationWorkspace->readX(0);
    MantidVec correlatedCounts = correlationWorkspace->readY(0);
    g_log.information() << "   Auto-correlation data read." << std::endl;

    setMinimumDistance(getProperty("MinimumPeakSeparation"));
    setMinimumPeakHeight(getProperty("MinimumPeakHeight"));
    setMaximumPeakNumber(getProperty("MaximumPeakNumber"));

    if(m_doubleMinimumDistance > static_cast<int>(correlatedCounts.size())) {
        throw(std::runtime_error("MinimumPeakSeparation is smaller than number of spectrum points - no peaks possible."));
    }

    g_log.information() << "   Parameters set." << std::endl;

    MantidVec summedNeighborCounts = getNeighborSums(correlatedCounts);
    g_log.information() << "   Neighboring counts summed, contains " << summedNeighborCounts.size() << " data points." << std::endl;

    std::list<MantidVec::iterator> peakPositionsSummed = findPeaks(summedNeighborCounts.begin(), summedNeighborCounts.end());
    g_log.information() << "   Peaks detected in summed spectrum: " << peakPositionsSummed.size() << std::endl;

    /* This step is required because peaks are actually searched in the "sum-of-neighbors"-spectrum.
     * The mapping removes the offset from the peak position which results from different beginning
     * of this vector compared to the original correlation counts.
     */
    std::list<MantidVec::iterator> peakPositionsCorrelation = mapPeakPositionsToCorrelationData(peakPositionsSummed, summedNeighborCounts.begin(), correlatedCounts.begin());
    g_log.information() << "   Peak positions transformed to original spectrum." << std::endl;

    /* Since intensities are required for filtering, they are extracted from the original count data,
     * along with the Q-values, forming "coordinates" (hence the use of V2D).
     */
    std::vector<V2D> peakCoordinates = getPeakCoordinates(summedNeighborCounts.begin(), peakPositionsSummed, correlationQValues);
    g_log.information() << "   Extracted peak positions in Q and intensity guesses." << std::endl;

    std::pair<double, double> backgroundWithSigma = getBackgroundWithSigma(peakPositionsCorrelation, correlatedCounts);
    g_log.information() << "   Calculated average background and deviation: " << backgroundWithSigma.first << ", " << backgroundWithSigma.second << std::endl;

    if((*getProperty("MinimumPeakHeight")).isDefault()) {
        setMinimumPeakHeight(minimumPeakHeightFromBackground(backgroundWithSigma));
    }

    std::vector<V2D> intensityFilteredPeaks(peakCoordinates.size());
    auto newEnd = std::remove_copy_if(peakCoordinates.begin(), peakCoordinates.end(), intensityFilteredPeaks.begin(), boost::bind<bool>(&PoldiPeakSearch::isLessThanMinimum, this, _1));
    intensityFilteredPeaks.resize(std::distance(intensityFilteredPeaks.begin(), newEnd));

    g_log.information() << "   Peaks above minimum intensity (" << m_minimumPeakHeight << "): " << intensityFilteredPeaks.size() << std::endl;

    std::sort(intensityFilteredPeaks.begin(), intensityFilteredPeaks.end(), &PoldiPeakSearch::yGreaterThan);


    ITableWorkspace_sptr peaks = boost::dynamic_pointer_cast<ITableWorkspace>(WorkspaceFactory::Instance().createTable());

    peaks->addColumn("double", "Q");
    peaks->addColumn("double", "Counts (estimated)");

    for(std::vector<V2D>::const_iterator peak = intensityFilteredPeaks.begin();
        peak != intensityFilteredPeaks.end();
        ++peak)
    {
        TableRow newRow = peaks->appendRow();
        newRow << (*peak).X() << (*peak).Y();
    }

    setErrorsOnWorkspace(correlationWorkspace, backgroundWithSigma.second);

    setProperty("OutputWorkspace", peaks);
}


} // namespace Poldi
} // namespace Mantid
