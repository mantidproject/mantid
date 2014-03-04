#include "MantidSINQ/PoldiPeakSearch.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/V2D.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

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

MantidVec PoldiPeakSearch::getNeighborSums(MantidVec correlatedCounts)
{
    /* Since the first and last element in a list don't have two neighbors, they are excluded from the calculation
     * and the result vector's size is reduced by two. Also, the algorithm does not work on vectors with fewer
     * than three elements.
     **/
    size_t counts = correlatedCounts.size();

    if(counts < 3) {
        throw std::runtime_error("A vector with less than three elements can not be processed.");
    }

    size_t validCounts = counts - 2;

    size_t n = 1;
    std::vector<size_t> validIndices(validCounts);
    std::generate(validIndices.begin(), validIndices.end(), [&n] () { return n++; });

    MantidVec summedNeighborCounts(validCounts);
    std::transform(validIndices.cbegin(), validIndices.cend(), summedNeighborCounts.begin(), [&correlatedCounts](size_t i) { return correlatedCounts[i - 1] + correlatedCounts[i] + correlatedCounts[i + 1]; });

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
    rawPeaks.sort([](MantidVec::iterator first, MantidVec::iterator second) { return (*first) > (*second); } );

    int actualPeakCount = std::min(m_maximumPeakNumber, static_cast<int>(rawPeaks.size()));

    std::list<MantidVec::iterator> truncatedPeaks(actualPeakCount);
    std::copy_n(rawPeaks.begin(), actualPeakCount, truncatedPeaks.begin());

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

std::list<MantidVec::iterator> PoldiPeakSearch::getOriginalDataPeakIterators(std::list<MantidVec::iterator> peakPositions, MantidVec::iterator baseDataStart, MantidVec::iterator originalDataStart)
{
    std::list<MantidVec::iterator> transformedIndices(peakPositions.size());
    std::transform(peakPositions.cbegin(), peakPositions.cend(), transformedIndices.begin(), [baseDataStart, originalDataStart](MantidVec::iterator summedDataIterator) { return originalDataStart + std::distance(baseDataStart, summedDataIterator) + 1; });

    return transformedIndices;
}

std::vector<V2D> PoldiPeakSearch::getPeakCoordinates(MantidVec::iterator baseListStart, std::list<MantidVec::iterator> peakPositions, MantidVec xData)
{
    std::vector<V2D> peakData;
    peakData.reserve(peakPositions.size());

    for(std::list<MantidVec::iterator>::const_iterator peak = peakPositions.cbegin();
        peak != peakPositions.cend();
        ++peak)
    {
        size_t index = std::distance(baseListStart, *peak) + 1;
        peakData.push_back(V2D(xData[index], **peak / 3.0));
    }

    return peakData;
}

size_t PoldiPeakSearch::getNumberOfBackgroundPoints(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts)
{
    size_t totalDataPoints = correlationCounts.size() - 2;
    size_t occupiedByPeaks = peakPositions.size() * (m_doubleMinimumDistance - 1);

    if(occupiedByPeaks > totalDataPoints) {
        throw(std::runtime_error("More data points occupied by peaks than existing data points - not possible."));
    }

    return totalDataPoints - occupiedByPeaks;
}

std::pair<double, double> PoldiPeakSearch::getBackgroundWithSigma(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts)
{
    // subtracting 2, to match original algorithm
    size_t backgroundPoints = getNumberOfBackgroundPoints(peakPositions, correlationCounts);

    MantidVec sigma;
    sigma.reserve(backgroundPoints);

    MantidVec background;
    background.reserve(backgroundPoints);

    for(MantidVec::iterator point = correlationCounts.begin() + 1;
        point != correlationCounts.end() - 1;
        ++point)
    {
        if(std::all_of(peakPositions.cbegin(), peakPositions.cend(), [point, this] (MantidVec::iterator peakPos) { return std::abs(std::distance(point, peakPos)) >= m_minimumDistance; })) {
            background.push_back(*point);
            sigma.push_back(std::abs(*(point - 1) - *point));
        }
    }

    double sumBackground = std::accumulate(background.cbegin(), background.cend(), 0.0);
    double sumSigma = std::accumulate(sigma.cbegin(), sigma.cend(), 0.0);

    return std::make_pair(sumBackground / static_cast<double>(background.size()), sumSigma / static_cast<double>(sigma.size()));
}

double PoldiPeakSearch::defaultMinimumPeakHeight(std::pair<double, double> backgroundWithSigma)
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

void PoldiPeakSearch::init()
{
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace", "", Direction::Input),
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

    Workspace2D_const_sptr correlationWorkspace = getProperty("InputWorkspace");
    setMinimumDistance(getProperty("MinimumPeakSeparation"));
    setMinimumPeakHeight(getProperty("MinimumPeakHeight"));
    setMaximumPeakNumber(getProperty("MaximumPeakNumber"));
    g_log.information() << "   Parameters set." << std::endl;

    MantidVec correlatedCounts = correlationWorkspace->dataY(0);

    MantidVec summedNeighborCounts = getNeighborSums(correlatedCounts);
    g_log.information() << "   Neighboring counts summed, contains " << summedNeighborCounts.size() << " data points." << std::endl;

    std::list<MantidVec::iterator> peakPositionsSummed = findPeaks(summedNeighborCounts.begin(), summedNeighborCounts.end());
    g_log.information() << "   Peaks detected in summed spectrum: " << peakPositionsSummed.size() << std::endl;

    std::list<MantidVec::iterator> peakPositionsCorrelation = getOriginalDataPeakIterators(peakPositionsSummed, summedNeighborCounts.begin(), correlatedCounts.begin());
    g_log.information() << "   Peak positions transformed to original spectrum." << std::endl;

    std::vector<V2D> peakCoordinates = getPeakCoordinates(summedNeighborCounts.begin(), peakPositionsSummed, correlationWorkspace->dataX(0));
    g_log.information() << "   Extracted peak positions in Q and intensity guesses." << std::endl;

    double realMinimumPeakHeight = m_minimumPeakHeight;

    if(realMinimumPeakHeight == 0.0) {
        std::pair<double, double> backgroundWithSigma = getBackgroundWithSigma(peakPositionsCorrelation, correlatedCounts);
        realMinimumPeakHeight = defaultMinimumPeakHeight(backgroundWithSigma);
    }

    std::vector<V2D> properPeaks(peakCoordinates.size());
    auto newEnd = std::copy_if(peakCoordinates.cbegin(), peakCoordinates.cend(), properPeaks.begin(), [&realMinimumPeakHeight] (V2D peak) { return peak.Y() > realMinimumPeakHeight; });
    properPeaks.resize(std::distance(properPeaks.begin(), newEnd));

    g_log.information() << "   Peaks above minimum intensity (" << realMinimumPeakHeight << "): " << properPeaks.size() << std::endl;

    std::sort(properPeaks.begin(), properPeaks.end(), [](V2D first, V2D second) { return first.Y() > second.Y(); });


    ITableWorkspace_sptr peaks = boost::dynamic_pointer_cast<ITableWorkspace>(WorkspaceFactory::Instance().createTable());

    peaks->addColumn("double", "Q");
    peaks->addColumn("double", "estimated Intensity");

    for(std::vector<V2D>::const_iterator peak = properPeaks.cbegin();
        peak != properPeaks.cend();
        ++peak)
    {
        TableRow newRow = peaks->appendRow();
        newRow << (*peak).X() << (*peak).Y();
    }

    setProperty("OutputWorkspace", peaks);
}


} // namespace Poldi
} // namespace Mantid
