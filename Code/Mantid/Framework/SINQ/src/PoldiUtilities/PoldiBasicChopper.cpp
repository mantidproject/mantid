#include "MantidSINQ/PoldiUtilities/PoldiBasicChopper.h"

#include "MantidGeometry/ICompAssembly.h"
#include "boost/bind.hpp"

namespace Mantid {
namespace Poldi {

PoldiBasicChopper::PoldiBasicChopper()
    : m_slitPositions(), m_distanceFromSample(0.0),

      m_rawt0(0.0), m_rawt0const(0.0),

      m_slitTimes(),

      m_rotationSpeed(0.0), m_cycleTime(0.0), m_zeroOffset(0.0) {}

void PoldiBasicChopper::loadConfiguration(
    Geometry::Instrument_const_sptr poldiInstrument) {
  Geometry::ICompAssembly_const_sptr chopperGroup =
      boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
          poldiInstrument->getComponentByName(std::string("chopper")));

  size_t numberOfSlits = chopperGroup->nelements();

  std::vector<double> slitPositions(numberOfSlits);
  for (size_t i = 0; i < numberOfSlits; ++i) {
    slitPositions[i] =
        chopperGroup->getChild(static_cast<const int>(i))->getPos().X();
  }

  double distance = chopperGroup->getPos().norm() * 1000.0;
  double t0 = chopperGroup->getNumberParameter("t0").front();
  double t0const = chopperGroup->getNumberParameter("t0_const").front();

  initializeFixedParameters(slitPositions, distance, t0, t0const);
}

void PoldiBasicChopper::setRotationSpeed(double rotationSpeed) {
  initializeVariableParameters(rotationSpeed);
}

const std::vector<double> &PoldiBasicChopper::slitPositions() {
  return m_slitPositions;
}

const std::vector<double> &PoldiBasicChopper::slitTimes() {
  return m_slitTimes;
}

double PoldiBasicChopper::rotationSpeed() { return m_rotationSpeed; }

double PoldiBasicChopper::cycleTime() { return m_cycleTime; }

double PoldiBasicChopper::zeroOffset() { return m_zeroOffset; }

double PoldiBasicChopper::distanceFromSample() { return m_distanceFromSample; }

void
PoldiBasicChopper::initializeFixedParameters(std::vector<double> slitPositions,
                                             double distanceFromSample,
                                             double t0, double t0const) {
  m_slitPositions.resize(slitPositions.size());
  std::copy(slitPositions.begin(), slitPositions.end(),
            m_slitPositions.begin());

  m_distanceFromSample = distanceFromSample;
  m_rawt0 = t0;
  m_rawt0const = t0const;
}

void PoldiBasicChopper::initializeVariableParameters(double rotationSpeed) {
  m_rotationSpeed = rotationSpeed;
  m_cycleTime = 60.0 / (4.0 * rotationSpeed) * 1.0e6;
  m_zeroOffset = m_rawt0 * m_cycleTime + m_rawt0const;

  m_slitTimes.resize(m_slitPositions.size());
  std::transform(m_slitPositions.begin(), m_slitPositions.end(),
                 m_slitTimes.begin(),
                 boost::bind<double>(
                     &PoldiBasicChopper::slitPositionToTimeFraction, this, _1));
}

double PoldiBasicChopper::slitPositionToTimeFraction(double slitPosition) {
  return slitPosition * m_cycleTime;
}
}
// namespace Poldi
} // namespace Mantid
