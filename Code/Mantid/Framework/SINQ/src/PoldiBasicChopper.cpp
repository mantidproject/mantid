#include "MantidSINQ/PoldiBasicChopper.h"

namespace Mantid
{
namespace Poldi
{


PoldiBasicChopper::PoldiBasicChopper() :
    m_slitPositions(),
    m_distanceFromSample(0.0),

    m_rawt0(0.0),
    m_rawt0const(0.0),

    m_slitTimes(),

    m_rotationSpeed(0.0),
    m_cycleTime(0.0),
    m_zeroOffset(0.0)
{
}

void PoldiBasicChopper::loadConfiguration(DataObjects::TableWorkspace_sptr chopperConfigurationWorkspace,
                                          DataObjects::TableWorkspace_sptr chopperSlitWorkspace,
                                          DataObjects::TableWorkspace_sptr chopperSpeedWorkspace)
{
    try {
        size_t rowIndex = -1;

        chopperConfigurationWorkspace->find(std::string("dist-chopper-sample"), rowIndex, 0);
        double chopperDistance = chopperConfigurationWorkspace->cell<double>(rowIndex, 2);

        chopperConfigurationWorkspace->find(std::string("t0"), rowIndex, 0);
        double rawt0 = chopperConfigurationWorkspace->cell<double>(rowIndex, 2);

        chopperConfigurationWorkspace->find(std::string("tconst"), rowIndex, 0);
        double rawt0const = chopperConfigurationWorkspace->cell<double>(rowIndex, 2);

        std::vector<double> chopperSlitVector = chopperSlitWorkspace->getColVector<double>(std::string("position"));

        chopperSpeedWorkspace->find(std::string("ChopperSpeed"), rowIndex, 0);
        double chopperSpeed = boost::lexical_cast<double>(chopperSpeedWorkspace->cell<std::string>(rowIndex, 2));

        initializeFixedParameters(chopperSlitVector, chopperDistance, rawt0, rawt0const);
        initializeVariableParameters(chopperSpeed);
    }
    catch(std::out_of_range&)
    {
        throw std::runtime_error("Missing configuration item for PoldiBasicChopper.");
    }
}

void PoldiBasicChopper::setRotationSpeed(double rotationSpeed)
{
    initializeVariableParameters(rotationSpeed);
}

std::vector<double> PoldiBasicChopper::slitPositions()
{
    return m_slitPositions;
}

std::vector<double> PoldiBasicChopper::slitTimes()
{
    return m_slitTimes;
}

double PoldiBasicChopper::rotationSpeed()
{
    return m_rotationSpeed;
}

double PoldiBasicChopper::cycleTime()
{
    return m_cycleTime;
}

double PoldiBasicChopper::zeroOffset()
{
    return m_zeroOffset;
}

double PoldiBasicChopper::distanceFromSample()
{
    return m_distanceFromSample;
}

void PoldiBasicChopper::initializeFixedParameters(std::vector<double> slitPositions, double distanceFromSample, double t0, double t0const)
{
    m_slitPositions.resize(slitPositions.size());
    std::copy(slitPositions.begin(), slitPositions.end(), m_slitPositions.begin());

    m_distanceFromSample = distanceFromSample;
    m_rawt0 = t0;
    m_rawt0const = t0const;
}

void PoldiBasicChopper::initializeVariableParameters(double rotationSpeed)
{
    m_rotationSpeed = rotationSpeed;
    m_cycleTime = 60.0 / (4.0 * rotationSpeed) * 1.0e6;
    m_zeroOffset = m_rawt0 * m_cycleTime + m_rawt0const;

    m_slitTimes.resize(m_slitPositions.size());
    std::transform(m_slitPositions.begin(), m_slitPositions.end(), m_slitTimes.begin(),
                   [this](double slitPosition) { return slitPosition * m_cycleTime; });
}


}
// namespace Poldi
} // namespace Mantid
