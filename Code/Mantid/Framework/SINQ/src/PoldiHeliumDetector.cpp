#include "MantidSINQ/PoldiHeliumDetector.h"

#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Poldi {

PoldiHeliumDetector::PoldiHeliumDetector() :
    PoldiAbstractDetector(),
    m_radius(0.0),
    m_elementCount(0),
    m_centralElement(0),
    m_elementWidth(0.0),
    m_angularResolution(0.0),
    m_totalOpeningAngle(0.0),
    m_calibratedPosition(0.0, 0.0),
    m_vectorAngle(0.0),
    m_distanceFromSample(0.0),
    m_calibratedCenterTwoTheta(0.0),
    m_phiCenter(0.0),
    m_phiStart(0.0)
{
}

void PoldiHeliumDetector::loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace)
{
    try {
        size_t rowIndex = -1;

        detectorConfigurationWorkspace->find(std::string("det_radius"), rowIndex, 0);
        double radius = detectorConfigurationWorkspace->cell<double>(rowIndex, 2);

        detectorConfigurationWorkspace->find(std::string("det_nb_channel"), rowIndex, 0);
        size_t elementCount = static_cast<size_t>(detectorConfigurationWorkspace->cell<double>(rowIndex, 2));

        detectorConfigurationWorkspace->find(std::string("det_channel_resolution"), rowIndex, 0);
        double elementWidth = detectorConfigurationWorkspace->cell<double>(rowIndex, 2);

        detectorConfigurationWorkspace->find(std::string("x0det"), rowIndex, 0);
        double x0det = detectorConfigurationWorkspace->cell<double>(rowIndex, 2);

        detectorConfigurationWorkspace->find(std::string("y0det"), rowIndex, 0);
        double y0det = detectorConfigurationWorkspace->cell<double>(rowIndex, 2);

        detectorConfigurationWorkspace->find(std::string("twothet"), rowIndex, 0);
        double twoTheta = detectorConfigurationWorkspace->cell<double>(rowIndex, 2) / 180.0 * M_PI;

        initializeFixedParameters(radius, elementCount, elementWidth);
        initializeCalibratedParameters(V2D(x0det, y0det), twoTheta);
    }
    catch(std::out_of_range& )
    {
        throw std::runtime_error("Missing configuration item for PoldiHeliumDetector");
    }
}

double PoldiHeliumDetector::twoTheta(int elementIndex)
{
    double phiForElement = phi(elementIndex);

    return atan2(m_calibratedPosition.Y() + m_radius * sin(phiForElement), m_calibratedPosition.X() + m_radius * cos(phiForElement));
}

double PoldiHeliumDetector::distanceFromSample(int elementIndex)
{
    return sqrt(pow(m_radius, 2.0) + pow(m_distanceFromSample, 2.0) - 2.0 * m_radius * m_distanceFromSample * cos(phi(elementIndex) - m_vectorAngle));
}

size_t PoldiHeliumDetector::elementCount()
{
    return m_elementCount;
}

size_t PoldiHeliumDetector::centralElement()
{
    return m_centralElement;
}

std::pair<double, double> PoldiHeliumDetector::qLimits(double lambdaMin, double lambdaMax)
{
    return std::pair<double, double>(4.0 * M_PI / lambdaMax * sin(twoTheta(0) / 2.0),
                                     4.0 * M_PI / lambdaMin * sin(twoTheta(m_elementCount - 1) / 2.0));
}

double PoldiHeliumDetector::phi(int elementIndex)
{
    return m_phiStart + (static_cast<double>(elementIndex) + 0.5) * m_angularResolution;
}

double PoldiHeliumDetector::phi(double twoTheta)
{
    return twoTheta - asin(m_distanceFromSample / m_radius * sin(M_PI + m_vectorAngle - twoTheta));
}

void PoldiHeliumDetector::initializeFixedParameters(double radius, size_t elementCount, double elementWidth)
{
    m_radius = radius;
    m_elementCount = elementCount;
    m_centralElement = (elementCount - 1) / 2;
    m_elementWidth = elementWidth;

    m_angularResolution = m_elementWidth / m_radius;
    m_totalOpeningAngle = static_cast<double>(m_elementCount) * m_angularResolution;
}

void PoldiHeliumDetector::initializeCalibratedParameters(Mantid::Kernel::V2D position, double centerTwoTheta)
{
    m_calibratedPosition = position;
    m_vectorAngle = atan(m_calibratedPosition.Y() / m_calibratedPosition.X());
    m_distanceFromSample = m_calibratedPosition.norm();

    m_calibratedCenterTwoTheta = centerTwoTheta;

    m_phiCenter = phi(m_calibratedCenterTwoTheta);
    m_phiStart = m_phiCenter - m_totalOpeningAngle / 2.0;
}

}
}
