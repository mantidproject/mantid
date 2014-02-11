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

void PoldiAutoCorrelationCore::setDeadWires(std::set<int> deadWireSet)
{
    m_deadWires = deadWireSet;
}

void PoldiAutoCorrelationCore::setWavelengthRange(double lambdaMin, double lambdaMax)
{
    m_wavelengthRange = std::make_pair(lambdaMin, lambdaMax);
}

std::pair<std::vector<double>, std::vector<double> > PoldiAutoCorrelationCore::calculate(std::vector<double> timeData, std::vector<double> countData)
{
    m_deltaT = timeData[1] - timeData[0];
    m_timeElements = timeData.size();

    // Create detector elements
    std::vector<int> rawElements = getRawElements();

    // filter out dead wires
    std::vector<int> goodElements = getGoodElements(rawElements);

    // Map element indices to 2Theta-Values
    std::vector<double> twoThetas(goodElements.size());
    std::transform(goodElements.begin(), goodElements.end(), twoThetas.begin(), boost::bind<double>(&PoldiAbstractDetector::twoTheta, m_detector, _1));

    // We will need sin(Theta) anyway, so we might just calculate those as well
    std::vector<double> sinThetas(goodElements.size());
    std::transform(twoThetas.begin(), twoThetas.end(), sinThetas.begin(), [](double twoTheta) { return sin(twoTheta / 2.0); });

    // Same goes for distances - map element index to distance, using detector object
    std::vector<double> distances(goodElements.size());
    std::transform(goodElements.begin(), goodElements.end(), distances.begin(), boost::bind<double>(&PoldiAbstractDetector::distanceFromSample, m_detector, _1));

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

std::vector<int> PoldiAutoCorrelationCore::getRawElements()
{
    size_t elementCount = m_detector->elementCount();

    std::vector<int> rawElements(elementCount);

    int i = 0;
    std::generate(rawElements.begin(), rawElements.end(), [&i] { return i++; });

    return rawElements;
}

std::vector<int> PoldiAutoCorrelationCore::getGoodElements(std::vector<int> rawElements)
{
    if(m_deadWires.size() > 0) {
        if(*m_deadWires.rbegin() > rawElements.back() + 1) {
            throw std::runtime_error(std::string("Deadwires set contains illegal index."));
        }
        size_t newElementCount = rawElements.size() - m_deadWires.size();

        std::vector<int> goodElements(newElementCount);
        std::remove_copy_if(rawElements.begin(), rawElements.end(), goodElements.begin(), [this](int index) { return m_deadWires.count(index + 1) != 0; });

        return goodElements;
    }

    return rawElements;
}

} // namespace Poldi
} // namespace Mantid
