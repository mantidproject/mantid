#include "MantidSINQ/PoldiAutoCorrelationCore.h"

#include <utility>
#include "boost/range/irange.hpp"
#include "boost/bind.hpp"
#include "MantidKernel/PhysicalConstants.h"

#include <iostream>
#include <fstream>

namespace Mantid
{
namespace Poldi
{

PoldiAutoCorrelationCore::PoldiAutoCorrelationCore() :
    m_detector(),
    m_chopper(),
    m_wavelengthRange(),
    m_deltaT(),
    m_timeElements(),
    m_detectorElements(),
    m_weightsForD(),
    m_tofsFor1Angstrom(),
    m_countData(),
    m_damp(0.0)
{
}

void PoldiAutoCorrelationCore::setInstrument(boost::shared_ptr<PoldiAbstractDetector> detector, boost::shared_ptr<PoldiAbstractChopper> chopper)
{
    m_detector = detector;
    m_chopper = chopper;
}

void PoldiAutoCorrelationCore::setWavelengthRange(double lambdaMin, double lambdaMax)
{
    m_wavelengthRange = std::make_pair(lambdaMin, lambdaMax);
}

void PoldiAutoCorrelationCore::calculate(DataObjects::Workspace2D_sptr countData,
                                         DataObjects::Workspace2D_sptr outputWorkspace)
{
    m_countData = countData;

    std::vector<double> timeData = countData->dataX(0);

    m_deltaT = timeData[1] - timeData[0];
    m_deltaD = getDeltaD(m_deltaT);
    m_timeElements = static_cast<int>(m_chopper->cycleTime() / m_deltaT);

    m_detectorElements = m_detector->availableElements();
    m_tofsFor1Angstrom = getTofsForD1(m_detectorElements);

    int n = 0;
    m_indices.resize(m_detectorElements.size());
    std::generate(m_indices.begin(), m_indices.end(), [&n] { return n++; });

    std::vector<double> dValues = getDGrid(m_deltaT);

    double sumOfWeights = getNormalizedTOFSum(m_tofsFor1Angstrom, m_deltaT, dValues.size());

    std::vector<double> rawCorrelatedIntensities(dValues.size());
    std::transform(dValues.cbegin(), dValues.cend(),
                   m_weightsForD.cbegin(),
                   rawCorrelatedIntensities.begin(),
                   boost::bind<double>(&PoldiAutoCorrelationCore::getRawCorrelatedIntensity, this, _1, _2));

    double sumOfCorrelatedIntensities = std::accumulate(rawCorrelatedIntensities.cbegin(), rawCorrelatedIntensities.cend(), 0.0);
    double sumOfCounts = getSumOfCounts(m_timeElements, m_detectorElements);

    double correlationBackground = sumOfCorrelatedIntensities - sumOfCounts;

    std::vector<double> correctedCorrelatedIntensities(dValues.size());
    std::transform(rawCorrelatedIntensities.cbegin(), rawCorrelatedIntensities.cend(),
                   m_weightsForD.cbegin(),
                   correctedCorrelatedIntensities.rbegin(),
                   [&correlationBackground, &sumOfWeights] (double intensity, double weight) { return intensity - correlationBackground * weight / sumOfWeights; });

    std::vector<double> qValues(dValues.size());
    std::transform(dValues.crbegin(), dValues.crend(), qValues.begin(), [] (double d) { return 2.0 * M_PI / d; });

    outputWorkspace->dataY(0) = correctedCorrelatedIntensities;

    outputWorkspace->setX(0, qValues);
    outputWorkspace->setX(1, qValues);
    outputWorkspace->setX(2, qValues);
}

double PoldiAutoCorrelationCore::getDeltaD(double deltaT)
{
    int centralElement = static_cast<int>(m_detector->centralElement());
    return TOFtod(deltaT, m_chopper->distanceFromSample() + m_detector->distanceFromSample(centralElement), sin(m_detector->twoTheta(centralElement) / 2.0));
}

std::pair<int, int> PoldiAutoCorrelationCore::getDRangeAsDeltaMultiples(double deltaD)
{
    std::pair<double, double> qLimits = m_detector->qLimits(m_wavelengthRange.first, m_wavelengthRange.second);

    return std::make_pair(static_cast<int>(2.0 * M_PI / qLimits.second / deltaD), static_cast<int>(2.0 * M_PI / qLimits.first / deltaD));
}

std::vector<double> PoldiAutoCorrelationCore::getDGrid(double deltaT)
{
    double deltaD = getDeltaD(deltaT);

    std::pair<int, int> normedDRange = getDRangeAsDeltaMultiples(deltaD);
    int ndSpace = normedDRange.second - normedDRange.first;

    std::vector<double> dGrid(ndSpace);

    double d0 = static_cast<double>(normedDRange.first) * deltaD;
    int n = 0;
    std::generate(dGrid.begin(), dGrid.end(), [&n, &deltaD, &d0]{ n++; return static_cast<double>(n) * deltaD + d0; });

    return dGrid;
}

double PoldiAutoCorrelationCore::getNormalizedTOFSum(std::vector<double> tofsForD1, double deltaT, size_t nd)
{
    /* Regarding numerical differences to fortran version
     *
     * In the latest version of the fortran software there is a bug that leads to dead
     * wires not being excluded and their contribution to the end result being counted.
     */

    calculateDWeights(tofsForD1, deltaT, nd);

    return std::accumulate(m_weightsForD.cbegin(), m_weightsForD.cend(), 0.0);
}

void PoldiAutoCorrelationCore::calculateDWeights(std::vector<double> tofsForD1, double deltaT, size_t nd)
{
    m_weightsForD.resize(nd);
    double deltaD = getDeltaD(deltaT);

    std::vector<double> tofs(tofsForD1.size());
    std::transform(tofsForD1.cbegin(), tofsForD1.cend(), tofs.begin(), [&deltaD](double tofForD1) { return tofForD1 * deltaD; });
    double sum = std::accumulate(tofs.begin(), tofs.end(), 0.0);

    std::fill(m_weightsForD.begin(), m_weightsForD.end(), sum / deltaT);
}

double PoldiAutoCorrelationCore::getNormalizedTOFSumAlternative(std::vector<double> tofsForD1, double deltaT, size_t nd)
{
    /* Unused algorithm
     *
     * This algorithm should in principle yield the same result as getNormalizedTOFSum,
     * but due to numerical differences (rounding, float vs. double, etc), it does not.
     * Since it is not used in the original code (anymore?), this will be here only for
     * reference purposes.
     */

    double deltaD = getDeltaD(deltaT);
    double ndd = static_cast<double>(nd);

    std::vector<double> tofs(tofsForD1.size());
    std::transform(tofsForD1.cbegin(), tofsForD1.cend(), tofs.begin(), [&deltaD, &ndd](double tofForD1) { return tofForD1 * ndd * deltaD; });

    return std::accumulate(tofs.begin(), tofs.end(), 0) / deltaT;
}

double PoldiAutoCorrelationCore::getRawCorrelatedIntensity(double dValue, double weight)
{
    std::vector<std::pair<double, double> > current;
    current.reserve(m_chopper->slitTimes().size());

    for(std::vector<double>::const_iterator slitOffset = m_chopper->slitTimes().cbegin();
        slitOffset != m_chopper->slitTimes().cend();
        ++slitOffset) {
        std::vector<std::pair<double, double> > cmess(m_detector->elementCount());
        std::transform(m_indices.cbegin(), m_indices.cend(),
                       cmess.begin(),
                       boost::bind<std::pair<double, double> >(&PoldiAutoCorrelationCore::getCMessAndCSigma, this, dValue, *slitOffset, _1));

        current.push_back(std::accumulate(cmess.cbegin(), cmess.cend(), std::make_pair(0.0, 0.0), [] (std::pair<double, double> sum, std::pair<double, double> current) { return std::make_pair(sum.first + current.first, sum.second + current.second); } ));
    }

    double sigma = (*std::min_element(current.cbegin(), current.cend(), [] (std::pair<double, double> first, std::pair<double, double> second) { return first.second < second.second; })).second;

    if(sigma <= 0) {
        return 0.0;
    }

    return reduceChopperSlitList(current, weight);
}

std::pair<double, double> PoldiAutoCorrelationCore::getCMessAndCSigma(double dValue, double slitTimeOffset, int index)
{
    int element = getElement(index);
    double tofFor1Angstrom = getTof(index);

    double rawCenter = (m_chopper->zeroOffset() + tofFor1Angstrom * dValue) / m_deltaT;
    double center = rawCenter - floor(rawCenter / static_cast<double>(m_timeElements)) * static_cast<double>(m_timeElements) + slitTimeOffset / m_deltaT;

    double width = tofFor1Angstrom * m_deltaD / m_deltaT;

    double cmin = center - width / 2.0;
    double cmax = center + width / 2.0;

    int icmin = static_cast<int>(floor(cmin));
    int icmax = static_cast<int>(floor(cmax));

    int iicmin = cleanIndex(icmin, m_timeElements);
    int iicmax = cleanIndex(icmax, m_timeElements);

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
            int middleIndex = cleanIndex((icmin + 1), m_timeElements);

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

double PoldiAutoCorrelationCore::reduceChopperSlitList(std::vector<std::pair<double, double> > valuesWithSigma, double weight)
{
    std::vector<double> iOverSigma(valuesWithSigma.size());
    std::transform(valuesWithSigma.cbegin(), valuesWithSigma.cend(), iOverSigma.begin(), [] (std::pair<double, double> iAndSigma) { return iAndSigma.first / iAndSigma.second; });

    if(std::any_of(iOverSigma.cbegin(), iOverSigma.cend(), [] (double iOverS) { return iOverS < 0; })) {
        return 0.0;
    }

    return pow(static_cast<double>(valuesWithSigma.size()), 2.0) / std::accumulate(iOverSigma.begin(), iOverSigma.end(), 0.0, [] (double sum, double curr) { return sum + 1.0 / curr; }) * weight;
}

std::vector<double> PoldiAutoCorrelationCore::getDistances(std::vector<int> elements)
{
    double chopperDistance = m_chopper->distanceFromSample();
    std::vector<double> distances(elements.size());
    std::transform(elements.begin(), elements.end(), distances.begin(), [this, &chopperDistance] (int element) { return chopperDistance + m_detector->distanceFromSample(element); });

    return distances;
}

std::vector<double> PoldiAutoCorrelationCore::getTofsForD1(std::vector<int> elements)
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

double PoldiAutoCorrelationCore::getCounts(int x, int y)
{
    return static_cast<double>(m_countData->dataY(399 - x)[y]);
}

double PoldiAutoCorrelationCore::getNormCounts(int x, int y)
{
    return std::max(1.0, static_cast<double>(m_countData->dataY(399 - x)[y]));
}

int PoldiAutoCorrelationCore::getElement(int index)
{
    return m_detectorElements[index];
}

double PoldiAutoCorrelationCore::getTof(int index)
{
    return m_tofsFor1Angstrom[index];
}

double PoldiAutoCorrelationCore::getSumOfCounts(int timeElements, std::vector<int> detectorElements)
{
    double sum = 0.0;

    for(int t = 0; t < timeElements; ++t) {
        for(std::vector<int>::const_iterator e = detectorElements.cbegin();
            e != detectorElements.cend();
            ++e) {
            sum += getCounts(*e, t);
        }
    }

    return sum;
}

int PoldiAutoCorrelationCore::cleanIndex(int index, int maximum)
{
    int cleanIndex = index % maximum;
    if(cleanIndex < 0) {
        cleanIndex += maximum;
    }

    return cleanIndex;
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

double PoldiAutoCorrelationCore::dtoTOF(double d, double distance, double sinTheta)
{

    return 2.0 * distance * sinTheta * d * PhysicalConstants::NeutronMass / (PhysicalConstants::h * 1e7);
}

double PoldiAutoCorrelationCore::TOFtod(double tof, double distance, double sinTheta)
{
    return PhysicalConstants::h * 1e7 * tof / (2.0 * distance * sinTheta * PhysicalConstants::NeutronMass);
}



} // namespace Poldi
} // namespace Mantid
