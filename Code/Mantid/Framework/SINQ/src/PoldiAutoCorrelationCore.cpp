#include "MantidSINQ/PoldiAutoCorrelationCore.h"

#include <utility>
#include "boost/range/irange.hpp"
#include "boost/bind.hpp"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid
{
namespace Poldi
{

PoldiAutoCorrelationCore::PoldiAutoCorrelationCore() :
    m_detector(),
    m_chopper()
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

std::pair<std::vector<double>, std::vector<double> > PoldiAutoCorrelationCore::calculate(std::vector<double> timeData, std::vector<double> countData)
{
    m_deltaT = timeData[1] - timeData[0];
    m_timeElements = timeData.size();

    std::vector<int> detectorElements = m_detector->availableElements();

    // Map element indices to 2Theta-Values
    std::vector<double> twoThetas(detectorElements.size());
    std::transform(detectorElements.begin(), detectorElements.end(), twoThetas.begin(), boost::bind<double>(&PoldiAbstractDetector::twoTheta, m_detector, _1));

    // We will need sin(Theta) anyway, so we might just calculate those as well
    std::vector<double> sinThetas(detectorElements.size());
    std::transform(twoThetas.begin(), twoThetas.end(), sinThetas.begin(), [](double twoTheta) { return sin(twoTheta / 2.0); });

    // Same goes for distances - map element index to distance, using detector object
    std::vector<double> distances(detectorElements.size());
    std::transform(detectorElements.begin(), detectorElements.end(), distances.begin(), boost::bind<double>(&PoldiAbstractDetector::distanceFromSample, m_detector, _1));

    // Time of flight for neutrons with a wavelength of 1 Angstrom for each element
    std::vector<double> tofFor1Angstrom(detectorElements.size());
    std::transform(distances.begin(), distances.end(), sinThetas.begin(), tofFor1Angstrom.begin(), boost::bind<double>(&PoldiAutoCorrelationCore::getTOFForD1, this, _1, _2));

}

double PoldiAutoCorrelationCore::getDeltaD(double deltaT)
{
    size_t centralElement = m_detector->centralElement();
    return   (PhysicalConstants::h / PhysicalConstants::NeutronMass / 1e-10)
           / (2.0 * (m_chopper->distanceFromSample() + m_detector->distanceFromSample(centralElement)) * sin(m_detector->twoTheta(centralElement) / 2.0))
            * deltaT * 1e-3;
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

double PoldiAutoCorrelationCore::getTOFForD1(double distance, double sinTheta)
{
    return 2./(PhysicalConstants::h / PhysicalConstants::NeutronMass / 1e-10) *1.e-7 * (m_chopper->distanceFromSample() + distance) * sinTheta;
}

} // namespace Poldi
} // namespace Mantid
