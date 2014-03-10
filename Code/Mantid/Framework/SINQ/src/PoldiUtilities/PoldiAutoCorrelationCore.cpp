#include "MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h"

#include <utility>
#include <numeric>
#include <algorithm>
#include "boost/range/irange.hpp"
#include "boost/bind.hpp"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
namespace Poldi
{

using namespace API;

PoldiAutoCorrelationCore::PoldiAutoCorrelationCore(Kernel::Logger &g_log) :
    m_detector(),
    m_chopper(),
    m_wavelengthRange(),
    m_deltaT(),
    m_timeBinCount(),
    m_detectorElements(),
    m_weightsForD(),
    m_tofsFor1Angstrom(),
    m_countData(),
    m_elementsMaxIndex(0),
    m_damp(0.0),
    m_logger(g_log)
{
}

/** Sets the components POLDI currently consists of. The detector should probably be one with a DeadWireDecorator so dead wires are taken into account properly.
  *
  * @param detector :: Instance of PoldiAbstractDetector.
  * @param chopper :: Instance of PoldiAbstractChopper.
  */
void PoldiAutoCorrelationCore::setInstrument(boost::shared_ptr<PoldiAbstractDetector> detector, boost::shared_ptr<PoldiAbstractChopper> chopper)
{
    m_detector = detector;
    m_chopper = chopper;

    m_logger.information() << "Detector and chopper assigned..." << std::endl;
}

/** Takes wavelength limits to be considered for the calculation.
  *
  * @param lambdaMin :: Minimum wavelength in Angstrom.
  * @param lambdaMax :: Maximum wavelength in Angstrom.
  */
void PoldiAutoCorrelationCore::setWavelengthRange(double lambdaMin, double lambdaMax)
{
    m_wavelengthRange = std::make_pair(lambdaMin, lambdaMax);
}

/** Performs auto-correlation algorithm on the POLDI data in the supplied workspace.
  *
  * @param countData :: Instance of Workspace2D with POLDI data.
  * @return A workspace containing the correlation spectrum.
  */
DataObjects::Workspace2D_sptr PoldiAutoCorrelationCore::calculate(DataObjects::Workspace2D_sptr countData)
{
    m_logger.information() << "Starting Autocorrelation method..." << std::endl;

    if(m_detector && m_chopper) {
        m_logger.information() << "  Assigning count data..." << std::endl;
        setCountData(countData);

        /* Calculations related to experiment timings
         *  - width of time bins (deltaT)
         *  - d-resolution deltaD, which results directly from deltaT
         *  - number of time bins for each copper cycle
         */
        std::vector<double> timeData = m_countData->dataX(0);

        m_logger.information() << "  Setting time data..." << std::endl;
        m_deltaT = timeData[1] - timeData[0];
        m_deltaD = getDeltaD(m_deltaT);
        m_timeBinCount = static_cast<int>(m_chopper->cycleTime() / m_deltaT);

        /* Data related to detector geometry
         *  - vector with available detector element-indices (wires, cells, ...)
         *  - vector that contains the TOF/Angstrom for each detector element
         *  - vector with indices on interval [0, number of detector elements) for help with access in algorithm
         */
        m_detectorElements = m_detector->availableElements();
        m_tofsFor1Angstrom = getTofsFor1Angstrom(m_detectorElements);

        int n = 0;
        m_indices.resize(m_detectorElements.size());
        std::generate(m_indices.begin(), m_indices.end(), [&n] { return n++; });

        /* The auto-correlation algorithm works by probing a list of d-Values, which
         * is created at this point. The spacing used is the maximum resolution of the instrument,
         * which was calculated before.
         */
        m_logger.information() << "  Generating d-grid..." << std::endl;
        std::vector<double> dValues = getDGrid(m_deltaD);

        /* When the correlation background is subtracted from the correlation spectrum, it is done for each d-Value
         * according to a certain weight. The calculation method corresponds closely to the original fortran program,
         * although it simply leads to unit weights.
         */
        m_logger.information() << "  Calculating weights (" << dValues.size() << ")..." << std::endl;
        m_weightsForD = calculateDWeights(m_tofsFor1Angstrom, m_deltaT, m_deltaD, dValues.size());
        double sumOfWeights = getNormalizedTOFSum(m_weightsForD);

        /* Calculation of the raw correlation spectrum. Each d-Value is mapped to an intensity value through,
         * taking into account the d-Value and the weight. Since the calculations for different d-Values do not
         * depend on eachother, the for-loop is marked as a candidate for parallelization. Since the calculation
         * of the intensity is rather complex and typical d-grids consist of ~5000 elements, the parallelization
         * pays off quite well.
         */
        m_logger.information() << "  Calculating intensities..." << std::endl;
        std::vector<double> rawCorrelatedIntensities(dValues.size());
        PARALLEL_FOR_NO_WSP_CHECK()
        for(size_t i = 0; i < dValues.size(); ++i) {
            rawCorrelatedIntensities[i] = getRawCorrelatedIntensity(dValues[i], m_weightsForD[i]);
        }

        /* As detailed in the original POLDI-paper, the sum of all correlation intensities is much higher than the
         * sum of counts in the recorded spectrum. The difference is called "correlation background" and is subtracted
         * from the raw intensities, using the weights calculated before.
         *
         * After this procedure, the sum of correlated intensities should be equal to the sum of counts in the spectrum.
         * In the fortran program there seems to be a small difference, possibly due to numerical inaccuracies connected
         * to floating point precision.
         */
        double sumOfCorrelatedIntensities = std::accumulate(rawCorrelatedIntensities.begin(), rawCorrelatedIntensities.end(), 0.0);
        double sumOfCounts = getSumOfCounts(m_timeBinCount, m_detectorElements);
        m_logger.information() << "  Summing intensities (" << sumOfCounts << ")..." << std::endl;

        double correlationBackground = sumOfCorrelatedIntensities - sumOfCounts;

        m_logger.information() << "  Correcting intensities..." << std::endl;
        std::vector<double> correctedCorrelatedIntensities(dValues.size());
        std::transform(rawCorrelatedIntensities.begin(), rawCorrelatedIntensities.end(),
                       m_weightsForD.begin(),
                       correctedCorrelatedIntensities.rbegin(),
                       [&correlationBackground, &sumOfWeights] (double intensity, double weight) { return intensity - correlationBackground * weight / sumOfWeights; });

        /* Finally, the d-Values are converted to q-Values for plotting etc. and inserted into the output workspace. */
        std::vector<double> qValues(dValues.size());
        std::transform(dValues.rbegin(), dValues.rend(), qValues.begin(), [] (double d) { return 2.0 * M_PI / d; });

        m_logger.information() << "  Setting result..." << std::endl;
        DataObjects::Workspace2D_sptr outputWorkspace = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
                (WorkspaceFactory::Instance().create("Workspace2D", 3, dValues.size(), dValues.size()));

        outputWorkspace->dataY(0) = correctedCorrelatedIntensities;

        outputWorkspace->setX(0, qValues);
        outputWorkspace->setX(1, qValues);
        outputWorkspace->setX(2, qValues);

        return outputWorkspace;
    } else {
        throw std::runtime_error("PoldiAutoCorrelationCore was run without specifying detector and chopper.");
    }
}
/** Computes the resolution limit of the POLDI experiment defined by the current instrument, in Angstrom, given the size of one time bin.
  *
  * Since this calculation is based on the time of flight, this value may be different for each point
  * of the detector, depending on the geometry. In the current implementation this is not taken into account,
  * instead the value for the center of the detector is calculated and assumed constant for the whole detector.
  *
  * @param deltaT :: Size of one time bin in microseconds.
  * @return Resolution in Angstrom corresponding to
  */
double PoldiAutoCorrelationCore::getDeltaD(double deltaT)
{
    int centralElement = static_cast<int>(m_detector->centralElement());
    return TOFtod(deltaT, m_chopper->distanceFromSample() + m_detector->distanceFromSample(centralElement), sin(m_detector->twoTheta(centralElement) / 2.0));
}

/** Computes a d-range from the limits set by the detector and expresses it as multiples of a step size given in Angstrom.
  *
  * @param deltaD :: Step size used for normalizing the calculated d-range (Angstrom).
  * @return Pair of integers containing the lower and upper d-limit, divided by deltaD.
  */
std::pair<int, int> PoldiAutoCorrelationCore::getDRangeAsDeltaMultiples(double deltaD)
{
    std::pair<double, double> qLimits = m_detector->qLimits(m_wavelengthRange.first, m_wavelengthRange.second);

    return std::make_pair(static_cast<int>(2.0 * M_PI / qLimits.second / deltaD), static_cast<int>(2.0 * M_PI / qLimits.first / deltaD));
}

/** Generates an equidistant grid of d-values with a given step size. The result depends on the assigned detector.
  *
  * @param deltaD :: Step size for the grid of d-values (Angstrom).
  * @return Vector containing all d-values (in Angstrom) that are possible to probe with the supplied instrument configuration.
  */
std::vector<double> PoldiAutoCorrelationCore::getDGrid(double deltaD)
{
    /* The algorithm used here is very close to the one used in the original fortran code. It actually does not start with the
     * smallest possible d-Value, but instead d0 + deltaD.
     */
    std::pair<int, int> normedDRange = getDRangeAsDeltaMultiples(deltaD);
    int ndSpace = normedDRange.second - normedDRange.first;

    std::vector<double> dGrid(ndSpace);

    double d0 = static_cast<double>(normedDRange.first) * deltaD;
    int n = 0;
    std::generate(dGrid.begin(), dGrid.end(), [&n, &deltaD, &d0]() -> double { n++; return static_cast<double>(n) * deltaD + d0; });

    return dGrid;
}

/** Computes the sums of the given vector. Currently simply wraps std::accumulate.
  *
  * @param normalizedTofs :: Vector with dimensionless numbers (TOF/(time bin size)).
  * @return Sum over all elements of given vector.
  */
double PoldiAutoCorrelationCore::getNormalizedTOFSum(std::vector<double> normalizedTofs)
{
    /* Regarding numerical differences to fortran version
     *
     * In the latest version of the fortran software there is a bug that leads to dead
     * wires not being excluded and their contribution to the end result being counted.
     * This can not happen here if a proper decorator is used on the detector
     * to handle dead wires.
     */

    return std::accumulate(normalizedTofs.begin(), normalizedTofs.end(), 0.0);
}

/** Calculates weights used for correction of correlation background.
  *
  * @param tofsFor1Angstrom :: Vector which contains the time of flight (in microseconds) for each element of the detector, for a neutron with lambda = 1 Angstrom.
  * @param deltaT :: Size of one time bin in microseconds.
  * @param deltaD :: d-Resolution in Angstrom.
  * @param nd :: Number of d-values.
  * @return Vector with nd elements, each containing the weight for the given d-Value
  */
std::vector<double> PoldiAutoCorrelationCore::calculateDWeights(std::vector<double> tofsFor1Angstrom, double deltaT, double deltaD, size_t nd)
{
    /* Currently, all d-values get the same weight, so in fact this calculation would is not really
     * necessary. But since there are not too many calculations involved and in order to stay close
     * to the original implementation, this is kept.
     */
    std::vector<double> tofs(tofsFor1Angstrom.size());
    std::transform(tofsFor1Angstrom.begin(), tofsFor1Angstrom.end(), tofs.begin(), [&deltaD](double tofForD1) { return tofForD1 * deltaD; });
    double sum = std::accumulate(tofs.begin(), tofs.end(), 0.0);

    return std::vector<double>(nd, sum / deltaT);
}

/** Returns correlation intensity for a given d-Value, using a given weight.
  *
  * @param dValue :: d-value in Angstrom.
  * @param weight :: Dimensionless weight used for reduction of intermediate result.
  * @return Correlated intensity, not corrected for correlation background.
  */
double PoldiAutoCorrelationCore::getRawCorrelatedIntensity(double dValue, double weight)
{
    /* Analysis according to the POLDI paper.
     *
     * For each d-value there is a contribution at each wire of the detector at a given time.
     * Since each chopper slit adds a small offset (between 0 and one cycletime) to the neutrons,
     * there are eight possible arrival "locations" (in the sense of both space and time) for neutrons
     * diffracted by this family of planes with given d.
     */
    std::vector<std::pair<double, double> > current;
    current.reserve(m_chopper->slitTimes().size());

    for(std::vector<double>::const_iterator slitOffset = m_chopper->slitTimes().begin();
        slitOffset != m_chopper->slitTimes().end();
        ++slitOffset) {
        /* For each offset, the sum of correlation intensity and error (for each detector element)
         * is computed from the counts in the space/time location possible for this d-value (by getCMessAndCSigma).
         * These pairs are put into a vector for later analysis. The size of this vector
         * is equal to the number of chopper slits.
         */
        std::vector<std::pair<double, double> > cmess(m_detector->elementCount());
        std::transform(m_indices.begin(), m_indices.end(),
                       cmess.begin(),
                       boost::bind<std::pair<double, double> >(&PoldiAutoCorrelationCore::getCMessAndCSigma, this, dValue, *slitOffset, _1));

        current.push_back(std::accumulate(cmess.begin(), cmess.end(), std::make_pair(0.0, 0.0), [] (std::pair<double, double> sum, std::pair<double, double> current) { return std::make_pair(sum.first + current.first, sum.second + current.second); } ));
    }

    /* This check ensures that all sigmas are non-zero. If not, a correlation intensity of 0.0 is returned. */
    double sigma = (*std::min_element(current.begin(), current.end(), [] (std::pair<double, double> first, std::pair<double, double> second) { return first.second < second.second; })).second;

    if(sigma <= 0) {
        return 0.0;
    }

    /* Finally, the list of I/sigma values is reduced to I.
     * The algorithm used for this depends on the intended use.
     */
    return reduceChopperSlitList(current, weight);
}

/** Calculate correlation intensity and error for a given d-Value and a given time-offset, arriving
  *
  * @param dValue :: d-value in Angstrom.
  * @param slitTimeOffset :: Time-offset given by chopperslit in microseconds.
  * @param index :: Index of detector element the calculation is performed for.
  * @return Pair of intensity and error for given input.
  */
std::pair<double, double> PoldiAutoCorrelationCore::getCMessAndCSigma(double dValue, double slitTimeOffset, int index)
{
    /* Element index and TOF for 1 Angstrom from current setup. */
    int element = getElementFromIndex(index);
    double tofFor1Angstrom = getTofFromIndex(index);

    /* Central time bin for given d-value in this wire, taking into account the offset resulting from chopper slit. */
    double rawCenter = (m_chopper->zeroOffset() + tofFor1Angstrom * dValue) / m_deltaT;
    double center = rawCenter - floor(rawCenter / static_cast<double>(m_timeBinCount)) * static_cast<double>(m_timeBinCount) + slitTimeOffset / m_deltaT;

    /* Since resolution in terms of d is limited, dValue is actually dValue +/- deltaD, so the arrival window
     * may be of different width.
     */
    double width = tofFor1Angstrom * m_deltaD / m_deltaT;

    /* From center and width, the indices of time bins that may be involved are derived.
     * Since the spectrum is periodic, the index wraps around. For accessing the count
     * data, integer indices are calculated and wrapped. For calculating the correlation terms,
     * floating point numbers are required.
     */
    double cmin = center - width / 2.0;
    double cmax = center + width / 2.0;

    int icmin = static_cast<int>(floor(cmin));
    int icmax = static_cast<int>(floor(cmax));

    int iicmin = cleanIndex(icmin, m_timeBinCount);
    int iicmax = cleanIndex(icmax, m_timeBinCount);

    /* In the original fortran program, three cases are considered for the width
     * of the arrival window: 1, 2 and 3 time bins (which corresponds to index differences of
     * 0, 1 and 2 - don't we all love situations where off-by-one error may be introduced?). Anything
     * larger than that is considered malformed and is discarded.
     *
     * For the three valid cases, intensity and error are calculated. In order to be
     * able to use different sources for different implementations, getCounts() and getNormCounts() are
     * provided.
     */
    int indexDifference = icmax - icmin;

    std::pair<double, double> c = std::make_pair(0.0, 0.0);

    double minCounts = getCounts(element, iicmin);
    double normMinCounts = getNormCounts(element, iicmin);

    switch(indexDifference) {
    case 0: {
            c.first = minCounts * width / normMinCounts;
            c.second = width / normMinCounts;
            break;
        }
    case 2: {
            int middleIndex = cleanIndex((icmin + 1), m_timeBinCount);

            double counts = getCounts(element, middleIndex);
            double normCounts = getNormCounts(element, middleIndex);

            c.first = counts * 1.0 / normCounts;
            c.second = 1.0 / normCounts;
        }
    case 1: {
            c.first += minCounts * (static_cast<double>(icmin) - cmin + 1.0) / normMinCounts;
            c.second += (static_cast<double>(icmin) - cmin + 1.0) / normMinCounts;

            double maxCounts = getCounts(element, iicmax);
            double normMaxCounts = getNormCounts(element, iicmax);

            c.first += maxCounts * (cmax - static_cast<double>(icmax)) / normMaxCounts;
            c.second += (cmax - static_cast<double>(icmax)) / normMaxCounts;
            break;
        }
    default:
        break;
    }

    return c;
}


/** Maps index I onto the interval [0, max - 1], wrapping around using modulo
  *
  * @param index :: Index I, on interval [-inf, inf]
  * @param maximum :: Number of indices.
  * @return Index on interval [0, max - 1]
  */
int PoldiAutoCorrelationCore::cleanIndex(int index, int maximum)
{
    int cleanIndex = index % maximum;
    if(cleanIndex < 0) {
        cleanIndex += maximum;
    }

    return cleanIndex;
}

/** Assigns workspace pointer containing count data to class member, stores maximum histogram index
  *
  * @param countData :: Workspace containing count data
  */
void PoldiAutoCorrelationCore::setCountData(DataObjects::Workspace2D_sptr countData)
{
    m_countData = countData;
    m_elementsMaxIndex = static_cast<int>(countData->getNumberHistograms()) - 1;
}

/** Reduces list of I/sigma-pairs for N chopper slits to correlation intensity by checking for negative I/sigma-ratios and summing their inverse values.
  *
  * @param valuesWithSigma :: Vector of I/sigma-pairs.
  * @param weight :: Dimensionless weight.
  * @return Correlated intensity, multiplied by weight.
  */
double PoldiAutoCorrelationCore::reduceChopperSlitList(std::vector<std::pair<double, double> > valuesWithSigma, double weight)
{
    std::vector<double> iOverSigma(valuesWithSigma.size());
    std::transform(valuesWithSigma.begin(), valuesWithSigma.end(), iOverSigma.begin(), [] (std::pair<double, double> iAndSigma) { return iAndSigma.first / iAndSigma.second; });

    if(*std::min_element(iOverSigma.begin(), iOverSigma.end()) < 0.0) {
        return 0.0;
    }

    return pow(static_cast<double>(valuesWithSigma.size()), 2.0) / std::accumulate(iOverSigma.begin(), iOverSigma.end(), 0.0, [] (double sum, double curr) { return sum + 1.0 / curr; }) * weight;
}

/** Transforms a vector of detector element indices to total flight path, adding the distances from chopper to sample and sample to detector element.
  *
  * The result depends on the detector that is used.
  *
  * @param elements :: Vector of element indices.
  * @return Vector of total neutron flight paths in mm.
  */
std::vector<double> PoldiAutoCorrelationCore::getDistances(std::vector<int> elements)
{
    double chopperDistance = m_chopper->distanceFromSample();
    std::vector<double> distances(elements.size());
    std::transform(elements.begin(), elements.end(), distances.begin(), [this, &chopperDistance] (int element) { return chopperDistance + m_detector->distanceFromSample(element); });

    return distances;
}

/** Transforms a vector of detector element indices to specific TOFs.
  *
  * The function takes a vector of detector element indices and calculates the time of flight for each detector element for neutrons with a wavelength of 1 Angstrom.
  *
  * @param elements :: Vector of element indices.
  * @return Vector of specific TOFs (microseconds/Angstrom).
  */
std::vector<double> PoldiAutoCorrelationCore::getTofsFor1Angstrom(std::vector<int> elements)
{
    // Map element indices to 2Theta-Values
    std::vector<double> twoThetas(elements.size());
    std::transform(elements.begin(), elements.end(), twoThetas.begin(), boost::bind<double>(&PoldiAbstractDetector::twoTheta, m_detector, _1));

    // We will need sin(Theta) anyway, so we might just calculate those as well
    std::vector<double> sinThetas(elements.size());
    std::transform(twoThetas.begin(), twoThetas.end(), sinThetas.begin(), [](double twoTheta) { return sin(twoTheta / 2.0); });

    // Same goes for distances
    std::vector<double> distances = getDistances(elements);

    // Time of flight for neutrons with a wavelength of 1 Angstrom for each element
    std::vector<double> tofFor1Angstrom(elements.size());
    std::transform(distances.begin(), distances.end(), sinThetas.begin(), tofFor1Angstrom.begin(), boost::bind<double>(&PoldiAutoCorrelationCore::dtoTOF, 1.0, _1, _2));

    return tofFor1Angstrom;
}

/** Returns counts at given position
  *
  * @param x :: Detector element index.
  * @param y :: Time bin index.
  * @return Counts at position.
  */
double PoldiAutoCorrelationCore::getCounts(int x, int y)
{
    return static_cast<double>(m_countData->dataY(m_elementsMaxIndex - x)[y]);
}

/** Returns normalized counts for correlation method at given position - these may come from a different source than the counts
  *
  * @param x :: Detector element index.
  * @param y :: Time bin index.
  * @return Normalized counts at position.
  */
double PoldiAutoCorrelationCore::getNormCounts(int x, int y)
{
    return std::max(1.0, static_cast<double>(m_countData->dataY(m_elementsMaxIndex - x)[y]));
}

/** Returns detector element index for given index
  *
  * This is a bit complicated at the moment, but currently necessary because this way dead wires are easier to handle
  *
  * @param index :: Detector element index.
  * @return Detector element index.
  */
int PoldiAutoCorrelationCore::getElementFromIndex(int index)
{
    if(index < 0 || index >= static_cast<int>(m_detectorElements.size())) {
        throw(std::range_error("Index out of bounds on accessing m_detectorElements."));
    }

    return m_detectorElements[index];
}

/** Returns specific TOF for given index
  *
  * @param index :: Specific TOF index.
  * @return TOF at index.
  */
double PoldiAutoCorrelationCore::getTofFromIndex(int index)
{
    if(index < 0 || index >= static_cast<int>(m_tofsFor1Angstrom.size())) {
        throw(std::range_error("Index out of bounds on accessing m_tofsFor1Angstrom."));
    }

    return m_tofsFor1Angstrom[index];
}

/** Returns the total sum of counts in the spectrum
  *
  * @param timeBinCount :: Number of time bins
  * @param detectorElements :: Vector with all detector elements to be considered for the summation.
  * @return Sum of all counts.
  */
double PoldiAutoCorrelationCore::getSumOfCounts(int timeBinCount, std::vector<int> detectorElements)
{
    double sum = 0.0;

    for(int t = 0; t < timeBinCount; ++t) {
        for(std::vector<int>::const_iterator e = detectorElements.begin();
            e != detectorElements.end();
            ++e) {
            sum += getCounts(*e, t);
        }
    }

    return sum;
}

/* Unit conversion functions dtoTOF and TOFtod
 *
 * This way of converting units leads to values that differ slightly from the ones produced
 * by the original fortran code. In that code there is a lot of "multiplying by 2PI and dividing by 2PI"
 * going on, which is not present here. These small deviations accumulate when a lot of conversions
 * are performed, so the end results may differ numerically in those cases.
 *
 * These two functions are exactly inverse, as demonstrated by the unit tests (PoldiAutoCorrelationCoreTest.h).
 */


/** Converts d to TOF, given a distance and sin(theta)
  *
  * @param d :: d in Angstrom.
  * @param distance :: Neutron flight path in mm.
  * @param sinTheta :: sin(theta).
  * @return TOF in microseconds.
  */
double PoldiAutoCorrelationCore::dtoTOF(double d, double distance, double sinTheta)
{

    return 2.0 * distance * sinTheta * d * PhysicalConstants::NeutronMass / (PhysicalConstants::h * 1e7);
}

/** Converts TOF to d, given a distance and sin(theta)
  *
  * @param tof :: Time of flight in microseconds.
  * @param distance :: Neutron flight path in mm.
  * @param sinTheta :: sin(theta).
  * @return d in Angstrom.
  */
double PoldiAutoCorrelationCore::TOFtod(double tof, double distance, double sinTheta)
{
    return PhysicalConstants::h * 1e7 * tof / (2.0 * distance * sinTheta * PhysicalConstants::NeutronMass);
}



} // namespace Poldi
} // namespace Mantid
