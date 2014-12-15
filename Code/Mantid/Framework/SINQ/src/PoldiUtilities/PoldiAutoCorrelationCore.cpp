#include "MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h"

#include <utility>
#include <numeric>
#include <algorithm>
#include "boost/bind.hpp"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MultiThreaded.h"

#include "MantidSINQ/PoldiUtilities/PoldiDGrid.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

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
    m_deltaD(),
    m_timeBinCount(),
    m_detectorElements(),
    m_weightsForD(),
    m_tofsFor1Angstrom(),
    m_countData(),
    m_normCountData(),
    m_sumOfWeights(0.0),
    m_correlationBackground(0.0),
    m_damp(0.0),
    m_logger(g_log)
{
}

/** Sets the components POLDI currently consists of. The detector should probably be one with a DeadWireDecorator so dead wires are taken into account properly.
  *
  * @param detector :: Instance of PoldiAbstractDetector.
  * @param chopper :: Instance of PoldiAbstractChopper.
  */
void PoldiAutoCorrelationCore::setInstrument(const PoldiAbstractDetector_sptr &detector, const PoldiAbstractChopper_sptr &chopper)
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

/** Finalizes the calculation of the correlation spectrum
  *
  * This method offers a variable way of using the correlation spectrum calculated previously.
  * The base version converts to Q and creates an appropriate output workspace.
  *
  * @param correctedCorrelatedIntensities :: Intensities of correlation spectrum.
  * @param dValues :: d-spacings at which the spectrum was calculated.
  * @return A workspace containing the correlation spectrum.
  */
DataObjects::Workspace2D_sptr PoldiAutoCorrelationCore::finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities, const std::vector<double> &dValues) const
{
    /* Finally, the d-Values are converted to q-Values for plotting etc. and inserted into the output workspace. */
    size_t dCount = dValues.size();
    std::vector<double> qValues(dCount);

    PARALLEL_FOR_NO_WSP_CHECK()
    for(int i = 0; i < static_cast<int>(dCount); ++i) {
        qValues[dCount - i - 1] = Conversions::dToQ(dValues[i]);
    }

    m_logger.information() << "  Setting result..." << std::endl;
    DataObjects::Workspace2D_sptr outputWorkspace = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D", 1, dValues.size(), dValues.size()));

    outputWorkspace->getAxis(0)->setUnit("MomentumTransfer");

    outputWorkspace->dataY(0) = correctedCorrelatedIntensities;

    outputWorkspace->setX(0, qValues);

    return outputWorkspace;
}

/** Performs auto-correlation algorithm on the POLDI data in the supplied workspace.
  *
  * @param countData :: Instance of Workspace2D with POLDI data.
  * @return A workspace containing the correlation spectrum.
  */
DataObjects::Workspace2D_sptr PoldiAutoCorrelationCore::calculate(DataObjects::Workspace2D_sptr &countData, const DataObjects::Workspace2D_sptr &normCountData)
{
    m_logger.information() << "Starting Autocorrelation method..." << std::endl;

    if(m_detector && m_chopper) {
        m_logger.information() << "  Assigning count data..." << std::endl;
        setCountData(countData);

        if(normCountData) {
            setNormCountData(normCountData);
        } else {
            setNormCountData(countData);
        }

        /* Calculations related to experiment timings
         *  - width of time bins (deltaT)
         *  - d-resolution deltaD, which results directly from deltaT
         *  - number of time bins for each copper cycle
         */
        std::vector<double> timeData = m_countData->readX(0);

        m_logger.information() << "  Setting time data..." << std::endl;
        m_deltaT = timeData[1] - timeData[0];
        m_timeBinCount = static_cast<int>(m_chopper->cycleTime() / m_deltaT);

        PoldiDGrid dGrid(m_detector, m_chopper, m_deltaT, m_wavelengthRange);

        m_deltaD = dGrid.deltaD();

        /* Data related to detector geometry
         *  - vector with available detector element-indices (wires, cells, ...)
         *  - vector that contains the TOF/Angstrom for each detector element
         *  - vector with indices on interval [0, number of detector elements) for help with access in algorithm
         */
        m_detectorElements = m_detector->availableElements();
        m_tofsFor1Angstrom = getTofsFor1Angstrom(m_detectorElements);

        m_indices.resize(m_detectorElements.size());
        for(int i = 0; i < static_cast<int>(m_indices.size()); ++i) {
            m_indices[i] = i;
        }

        /* The auto-correlation algorithm works by probing a list of d-Values, which
         * is created at this point. The spacing used is the maximum resolution of the instrument,
         * which was calculated before.
         */
        m_logger.information() << "  Generating d-grid..." << std::endl;
        std::vector<double> dValues = dGrid.grid();

        /* When the correlation background is subtracted from the correlation spectrum, it is done for each d-Value
         * according to a certain weight. The calculation method corresponds closely to the original fortran program,
         * although it simply leads to unit weights.
         */
        m_logger.information() << "  Calculating weights (" << dValues.size() << ")..." << std::endl;
        m_weightsForD = calculateDWeights(m_tofsFor1Angstrom, m_deltaT, m_deltaD, dValues.size());

        m_sumOfWeights = getNormalizedTOFSum(m_weightsForD);

        /* Calculation of the raw correlation spectrum. Each d-Value is mapped to an intensity value through,
         * taking into account the d-Value and the weight. Since the calculations for different d-Values do not
         * depend on eachother, the for-loop is marked as a candidate for parallelization. Since the calculation
         * of the intensity is rather complex and typical d-grids consist of ~5000 elements, the parallelization
         * pays off quite well.
         */
        m_logger.information() << "  Calculating intensities..." << std::endl;
        std::vector<double> rawCorrelatedIntensities(dValues.size());
        PARALLEL_FOR_NO_WSP_CHECK()
        for(int i = 0; i < static_cast<int>(dValues.size()); ++i) {
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

        m_correlationBackground = calculateCorrelationBackground(sumOfCorrelatedIntensities, sumOfCounts);

        m_logger.information() << "  Correcting intensities..." << std::endl;
        std::vector<double> correctedCorrelatedIntensities(dValues.size());
        std::transform(rawCorrelatedIntensities.begin(), rawCorrelatedIntensities.end(),
                       m_weightsForD.begin(),
                       correctedCorrelatedIntensities.rbegin(),
                       boost::bind<double>(&PoldiAutoCorrelationCore::correctedIntensity, this, _1, _2));

        /* The algorithm performs some finalization. In the default case the spectrum is
         * simply converted to Q and stored in a workspace. The reason to make this step variable
         * is that for calculation of residuals, everything is the same except this next step.
         */
        return finalizeCalculation(correctedCorrelatedIntensities, dValues);
    } else {
        throw std::runtime_error("PoldiAutoCorrelationCore was run without specifying detector and chopper.");
    }
}

/** Computes the sums of the given vector. Currently simply wraps std::accumulate.
  *
  * @param normalizedTofs :: Vector with dimensionless numbers (TOF/(time bin size)).
  * @return Sum over all elements of given vector.
  */
double PoldiAutoCorrelationCore::getNormalizedTOFSum(const std::vector<double> &normalizedTofs) const
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
std::vector<double> PoldiAutoCorrelationCore::calculateDWeights(const std::vector<double> &tofsFor1Angstrom, double deltaT, double deltaD, size_t nd) const
{
    /* Currently, all d-values get the same weight, so in fact this calculation would is not really
     * necessary. But since there are not too many calculations involved and in order to stay close
     * to the original implementation, this is kept.
     */
    std::vector<double> tofs;
    tofs.reserve(tofsFor1Angstrom.size());

    for(std::vector<double>::const_iterator tofFor1Angstrom = tofsFor1Angstrom.begin(); tofFor1Angstrom != tofsFor1Angstrom.end(); ++tofFor1Angstrom) {
        tofs.push_back(*tofFor1Angstrom * deltaD);
    }

    double sum = std::accumulate(tofs.begin(), tofs.end(), 0.0);

    return std::vector<double>(nd, sum / deltaT);
}

/** Returns correlation intensity for a given d-Value, using a given weight.
  *
  * @param dValue :: d-value in Angstrom.
  * @param weight :: Dimensionless weight used for reduction of intermediate result.
  * @return Correlated intensity, not corrected for correlation background.
  */
double PoldiAutoCorrelationCore::getRawCorrelatedIntensity(double dValue, double weight) const
{
    /* Analysis according to the POLDI paper.
     *
     * For each d-value there is a contribution at each wire of the detector at a given time.
     * Since each chopper slit adds a small offset (between 0 and one cycletime) to the neutrons,
     * there are eight possible arrival "locations" (in the sense of both space and time) for neutrons
     * diffracted by this family of planes with given d.
     */
    try {
        std::vector<UncertainValue> current;
        current.reserve(m_chopper->slitTimes().size());

        for(std::vector<double>::const_iterator slitOffset = m_chopper->slitTimes().begin();
            slitOffset != m_chopper->slitTimes().end();
            ++slitOffset) {
            /* For each offset, the sum of correlation intensity and error (for each detector element)
             * is computed from the counts in the space/time location possible for this d-value (by getCMessAndCSigma).
             * These pairs are put into a vector for later analysis. The size of this vector
             * is equal to the number of chopper slits.
             */
            std::vector<UncertainValue> cmess(m_detector->elementCount());
            std::transform(m_indices.begin(), m_indices.end(),
                           cmess.begin(),
                           boost::bind<UncertainValue>(&PoldiAutoCorrelationCore::getCMessAndCSigma, this, dValue, *slitOffset, _1));

            UncertainValue sum = std::accumulate(cmess.begin(), cmess.end(), UncertainValue(0.0, 0.0), &UncertainValue::plainAddition);

            current.push_back(sum);
        }

        /* Finally, the list of I/sigma values is reduced to I.
         * The algorithm used for this depends on the intended use.
         */
        return reduceChopperSlitList(current, weight);
    } catch (std::domain_error) {
        /* Trying to construct an UncertainValue with negative error will throw, so to preserve
         * the old "checking behavior", this exception is caught here.
         */
        return 0.0;
    }
}

/** Calculate correlation intensity and error for a given d-Value and a given time-offset, arriving
  *
  * @param dValue :: d-value in Angstrom.
  * @param slitTimeOffset :: Time-offset given by chopperslit in microseconds.
  * @param index :: Index of detector element the calculation is performed for.
  * @return Pair of intensity and error for given input.
  */
UncertainValue PoldiAutoCorrelationCore::getCMessAndCSigma(double dValue, double slitTimeOffset, int index) const
{
    /* The "count locator" describes where the counts for a given combination of d, time offset and 2-theta value
     * (corresponding to dValue, slitTimeOffset and index) can be found on the detector.
     */
    CountLocator locator = getCountLocator(dValue, slitTimeOffset, index);

    /* In the original fortran program, three cases are considered for the width
     * of the arrival window: 1, 2 and 3 time bins (which corresponds to index differences of
     * 0, 1 and 2 - don't we all love situations where off-by-one error may be introduced?). Anything
     * larger than that is considered malformed and is discarded.
     *
     * For the three valid cases, intensity and error are calculated. In order to be
     * able to use different sources for different implementations, getCounts() and getNormCounts() are
     * provided.
     */
    int indexDifference = locator.icmax - locator.icmin;

    double value = 0.0;
    double error = 0.0;

    double minCounts = getCounts(locator.detectorElement, locator.iicmin);
    double normMinCounts = getNormCounts(locator.detectorElement, locator.iicmin);

    switch(indexDifference) {
    case 0: {
            value = minCounts * locator.arrivalWindowWidth / normMinCounts;
            error = locator.arrivalWindowWidth / normMinCounts;
            break;
        }
    case 2: {
            int middleIndex = cleanIndex((locator.icmin + 1), m_timeBinCount);

            double counts = getCounts(locator.detectorElement, middleIndex);
            double normCounts = getNormCounts(locator.detectorElement, middleIndex);

            value = counts * 1.0 / normCounts;
            error = 1.0 / normCounts;
        }
    case 1: {
            value += minCounts * (static_cast<double>(locator.icmin) - locator.cmin + 1.0) / normMinCounts;
            error += (static_cast<double>(locator.icmin) - locator.cmin + 1.0) / normMinCounts;

            double maxCounts = getCounts(locator.detectorElement, locator.iicmax);
            double normMaxCounts = getNormCounts(locator.detectorElement, locator.iicmax);

            value += maxCounts * (locator.cmax - static_cast<double>(locator.icmax)) / normMaxCounts;
            error += (locator.cmax - static_cast<double>(locator.icmax)) / normMaxCounts;
            break;
        }
    default:
        break;
    }

    return UncertainValue(value, error);
}

/** Return parameters for locating counts in the data
  *
  * This method forms the heart of PoldiAutoCorrelationCore::getCMessAndCSigma, by returning
  * an object that contains the necessary information to locate counts in the stored data
  * that belong to a given combination of d, TOF-offset and detector element.
  *
  * @param dValue :: d-spacing that should be located.
  * @param slitTimeOffset :: TOF offset given by chopper slit.
  * @param index :: Index of detector element.
  * @return CountLocator object which contains information about the location of counts.
  */
CountLocator PoldiAutoCorrelationCore::getCountLocator(double dValue, double slitTimeOffset, int index) const
{
    CountLocator locator;
    /* Element index and TOF for 1 Angstrom from current setup. */
    locator.detectorElement = getElementFromIndex(index);
    double tofFor1Angstrom = getTofFromIndex(index);

    /* Central time bin for given d-value in this wire, taking into account the offset resulting from chopper slit. */
    double rawCenter = (m_chopper->zeroOffset() + tofFor1Angstrom * dValue) / m_deltaT;
    locator.arrivalWindowCenter = rawCenter - floor(rawCenter / static_cast<double>(m_timeBinCount)) * static_cast<double>(m_timeBinCount) + slitTimeOffset / m_deltaT;

    /* Since resolution in terms of d is limited, dValue is actually dValue +/- deltaD, so the arrival window
     * may be of different width.
     */
    locator.arrivalWindowWidth = tofFor1Angstrom * m_deltaD / m_deltaT;

    /* From center and width, the indices of time bins that may be involved are derived.
     * Since the spectrum is periodic, the index wraps around. For accessing the count
     * data, integer indices are calculated and wrapped. For calculating the correlation terms,
     * floating point numbers are required.
     */
    locator.cmin = locator.arrivalWindowCenter - locator.arrivalWindowWidth / 2.0;
    locator.cmax = locator.arrivalWindowCenter + locator.arrivalWindowWidth / 2.0;

    locator.icmin = static_cast<int>(floor(locator.cmin));
    locator.icmax = static_cast<int>(floor(locator.cmax));

    locator.iicmin = cleanIndex(locator.icmin, m_timeBinCount);
    locator.iicmax = cleanIndex(locator.icmax, m_timeBinCount);

    return locator;
}


/** Maps index I onto the interval [0, max - 1], wrapping around using modulo
  *
  * @param index :: Index I, on interval [-inf, inf]
  * @param maximum :: Number of indices.
  * @return Index on interval [0, max - 1]
  */
int PoldiAutoCorrelationCore::cleanIndex(int index, int maximum) const
{
    int cleanIndex = index % maximum;
    if(cleanIndex < 0) {
        cleanIndex += maximum;
    }

    return cleanIndex;
}

/** Assigns workspace pointer containing count data to class member
  *
  * @param countData :: Workspace containing count data
  */
void PoldiAutoCorrelationCore::setCountData(const DataObjects::Workspace2D_sptr &countData)
{
    m_countData = countData;
}


/** Assigns workspace pointer containing norm count data to class member
  *
  * @param countData :: Workspace containing norm count data
  */
void PoldiAutoCorrelationCore::setNormCountData(const DataObjects::Workspace2D_sptr &normCountData)
{
    m_normCountData = normCountData;
}

/** Returns the corrected intensity.
  *
  * This method returns the corrected intensity calculated from the supplied intensity value
  * and weight. It also uses the internally stored values for the correlation background
  * and the sum of weights for all d values.
  *
  * @param intensity :: Raw correlation intensity.
  * @param weight :: Weight as determined in calculateDWeights.
  * @return Corrected correlation intensity.
 */
double PoldiAutoCorrelationCore::correctedIntensity(double intensity, double weight) const
{
    return intensity - m_correlationBackground * weight / m_sumOfWeights;
}

double PoldiAutoCorrelationCore::calculateCorrelationBackground(double sumOfCorrelationCounts, double sumOfCounts) const
{
    return sumOfCorrelationCounts - sumOfCounts;
}

/** Reduces list of I/sigma-pairs for N chopper slits to correlation intensity by checking for negative I/sigma-ratios and summing their inverse values.
  *
  * @param valuesWithSigma :: Vector of I/sigma-pairs.
  * @param weight :: Dimensionless weight.
  * @return Correlated intensity, multiplied by weight.
  */
double PoldiAutoCorrelationCore::reduceChopperSlitList(const std::vector<UncertainValue> &valuesWithSigma, double weight) const
{
    try {
        std::vector<double> signalToNoise(valuesWithSigma.size());
        std::transform(valuesWithSigma.begin(), valuesWithSigma.end(), signalToNoise.begin(), &UncertainValue::errorToValueRatio);

        return pow(static_cast<double>(valuesWithSigma.size()), 2.0) / std::accumulate(signalToNoise.begin(), signalToNoise.end(), 0.0) * weight;
    } catch (std::domain_error) {
        return 0.0;
    }
}

/** Transforms a vector of detector element indices to total flight path, adding the distances from chopper to sample and sample to detector element.
  *
  * The result depends on the detector that is used.
  *
  * @param elements :: Vector of element indices.
  * @return Vector of total neutron flight paths in mm.
  */
std::vector<double> PoldiAutoCorrelationCore::getDistances(const std::vector<int> &elements) const
{
    double chopperDistance = m_chopper->distanceFromSample();
    std::vector<double> distances;
    distances.reserve(elements.size());

    for(std::vector<int>::const_iterator element = elements.begin(); element != elements.end(); ++element) {
        distances.push_back(chopperDistance + m_detector->distanceFromSample(*element));
    }

    return distances;
}

/** Transforms a vector of detector element indices to specific TOFs.
  *
  * The function takes a vector of detector element indices and calculates the time of flight for each detector element for neutrons with a wavelength of 1 Angstrom.
  *
  * @param elements :: Vector of element indices.
  * @return Vector of specific TOFs (microseconds/Angstrom).
  */
std::vector<double> PoldiAutoCorrelationCore::getTofsFor1Angstrom(const std::vector<int> &elements) const
{
    // Map element indices to 2Theta-Values
    std::vector<double> twoThetas(elements.size());
    std::transform(elements.begin(), elements.end(), twoThetas.begin(), boost::bind<double>(&PoldiAbstractDetector::twoTheta, m_detector, _1));

    // We will need sin(Theta) anyway, so we might just calculate those as well
    std::vector<double> sinThetas;
    sinThetas.reserve(elements.size());
    for(std::vector<double>::const_iterator twoTheta = twoThetas.begin(); twoTheta != twoThetas.end(); ++twoTheta) {
        sinThetas.push_back(sin(*twoTheta / 2.0));
    }

    // Same goes for distances
    std::vector<double> distances = getDistances(elements);

    // Time of flight for neutrons with a wavelength of 1 Angstrom for each element
    std::vector<double> tofFor1Angstrom(elements.size());
    std::transform(distances.begin(), distances.end(), sinThetas.begin(), tofFor1Angstrom.begin(), boost::bind<double>(&Conversions::dtoTOF, 1.0, _1, _2));

    return tofFor1Angstrom;
}

/** Returns counts at given position
  *
  * @param x :: Detector element index.
  * @param y :: Time bin index.
  * @return Counts at position.
  */
double PoldiAutoCorrelationCore::getCounts(int x, int y) const
{
    return m_countData->readY(x)[y];
}

/** Returns normalized counts for correlation method at given position - these may come from a different source than the counts
  *
  * @param x :: Detector element index.
  * @param y :: Time bin index.
  * @return Normalized counts at position.
  */
double PoldiAutoCorrelationCore::getNormCounts(int x, int y) const
{
    return std::max(1.0, m_normCountData->readY(x)[y]);
}

/** Returns detector element index for given index
  *
  * This is a bit complicated at the moment, but currently necessary because this way dead wires are easier to handle
  *
  * @param index :: Detector element index.
  * @return Detector element index.
  */
int PoldiAutoCorrelationCore::getElementFromIndex(int index) const
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
double PoldiAutoCorrelationCore::getTofFromIndex(int index) const
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
double PoldiAutoCorrelationCore::getSumOfCounts(int timeBinCount, const std::vector<int> &detectorElements) const
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

} // namespace Poldi
} // namespace Mantid
