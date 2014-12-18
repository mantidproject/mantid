#include "MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h"

#include "MantidGeometry/IComponent.h"

#include "MantidKernel/V3D.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"

namespace Mantid {
namespace Poldi {

using namespace Geometry;

PoldiHeliumDetector::PoldiHeliumDetector()
    : PoldiAbstractDetector(), m_radius(0.0), m_elementCount(0),
      m_centralElement(0), m_elementWidth(0.0), m_angularResolution(0.0),
      m_totalOpeningAngle(0.0), m_availableElements(), m_efficiency(0.0),
      m_calibratedPosition(0.0, 0.0), m_vectorAngle(0.0),
      m_distanceFromSample(0.0), m_calibratedCenterTwoTheta(0.0),
      m_phiCenter(0.0), m_phiStart(0.0) {}

void
PoldiHeliumDetector::loadConfiguration(Instrument_const_sptr poldiInstrument) {
  IComponent_const_sptr detector =
      poldiInstrument->getComponentByName("detector");
  double radius = detector->getNumberParameter("radius").front() * 1000.0;
  double elementWidth =
      detector->getNumberParameter("element_separation").front() * 1000.0;
  double newEfficiency = detector->getNumberParameter("efficiency").front();

  initializeFixedParameters(radius, poldiInstrument->getNumberDetectors(),
                            elementWidth, newEfficiency);

  Kernel::V3D pos = detector->getPos() * 1000.0;
  double twoTheta =
      Conversions::degToRad(detector->getNumberParameter("two_theta").front());
  initializeCalibratedParameters(Kernel::V2D(pos.X(), pos.Y()), twoTheta);
}

double PoldiHeliumDetector::efficiency() { return m_efficiency; }

double PoldiHeliumDetector::twoTheta(int elementIndex) {
  double phiForElement = phi(elementIndex);

  return atan2(m_calibratedPosition.Y() + m_radius * sin(phiForElement),
               m_calibratedPosition.X() + m_radius * cos(phiForElement));
}

double PoldiHeliumDetector::distanceFromSample(int elementIndex) {
  return sqrt(pow(m_radius, 2.0) + pow(m_distanceFromSample, 2.0) -
              2.0 * m_radius * m_distanceFromSample *
                  cos(phi(elementIndex) - m_vectorAngle));
}

size_t PoldiHeliumDetector::elementCount() { return m_elementCount; }

size_t PoldiHeliumDetector::centralElement() { return m_centralElement; }

const std::vector<int> &PoldiHeliumDetector::availableElements() {
  return m_availableElements;
}

std::pair<double, double> PoldiHeliumDetector::qLimits(double lambdaMin,
                                                       double lambdaMax) {
  return std::pair<double, double>(
      4.0 * M_PI / lambdaMax * sin(twoTheta(0) / 2.0),
      4.0 * M_PI / lambdaMin *
          sin(twoTheta(static_cast<int>(m_elementCount) - 1) / 2.0));
}

double PoldiHeliumDetector::phi(int elementIndex) {
  return m_phiStart +
         (static_cast<double>(elementIndex) + 0.5) * m_angularResolution;
}

double PoldiHeliumDetector::phi(double twoTheta) {
  return twoTheta - asin(m_distanceFromSample / m_radius *
                         sin(M_PI + m_vectorAngle - twoTheta));
}

void PoldiHeliumDetector::initializeFixedParameters(double radius,
                                                    size_t elementCount,
                                                    double elementWidth,
                                                    double newEfficiency) {
  m_efficiency = newEfficiency;
  m_radius = radius;
  m_elementCount = elementCount;
  m_centralElement = (elementCount - 1) / 2;
  m_elementWidth = elementWidth;

  m_availableElements.resize(m_elementCount);

  for (int i = 0; i < static_cast<int>(m_elementCount); ++i) {
    m_availableElements[i] = i;
  }

  m_angularResolution = m_elementWidth / m_radius;
  m_totalOpeningAngle =
      static_cast<double>(m_elementCount) * m_angularResolution;
}

void
PoldiHeliumDetector::initializeCalibratedParameters(Kernel::V2D position,
                                                    double centerTwoTheta) {
  m_calibratedPosition = position;
  m_vectorAngle = atan(m_calibratedPosition.Y() / m_calibratedPosition.X());
  m_distanceFromSample = m_calibratedPosition.norm();

  m_calibratedCenterTwoTheta = centerTwoTheta;

  m_phiCenter = phi(m_calibratedCenterTwoTheta);
  m_phiStart = m_phiCenter - m_totalOpeningAngle / 2.0;
}
}
}
