#include "MantidSINQ/PoldiUtilities/PoldiDetectorDecorator.h"

namespace Mantid {
namespace Poldi {

using namespace Geometry;

PoldiDetectorDecorator::PoldiDetectorDecorator(
    boost::shared_ptr<PoldiAbstractDetector> decoratedDetector)
    : PoldiAbstractDetector(), m_decoratedDetector() {
  setDecoratedDetector(decoratedDetector);
}

void PoldiDetectorDecorator::setDecoratedDetector(
    boost::shared_ptr<PoldiAbstractDetector> detector) {
  m_decoratedDetector = detector;

  detectorSetHook();
}

boost::shared_ptr<PoldiAbstractDetector>
PoldiDetectorDecorator::decoratedDetector() {
  return m_decoratedDetector;
}

void PoldiDetectorDecorator::loadConfiguration(
    Instrument_const_sptr poldiInstrument) {
  UNUSED_ARG(poldiInstrument)
}

double PoldiDetectorDecorator::efficiency() {
  if (m_decoratedDetector) {
    return m_decoratedDetector->efficiency();
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

double PoldiDetectorDecorator::twoTheta(int elementIndex) {
  if (m_decoratedDetector) {
    return m_decoratedDetector->twoTheta(elementIndex);
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

double PoldiDetectorDecorator::distanceFromSample(int elementIndex) {
  if (m_decoratedDetector) {
    return m_decoratedDetector->distanceFromSample(elementIndex);
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

size_t PoldiDetectorDecorator::elementCount() {
  if (m_decoratedDetector) {
    return m_decoratedDetector->elementCount();
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

size_t PoldiDetectorDecorator::centralElement() {
  if (m_decoratedDetector) {
    return m_decoratedDetector->centralElement();
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

const std::vector<int> &PoldiDetectorDecorator::availableElements() {
  if (m_decoratedDetector) {
    return m_decoratedDetector->availableElements();
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

std::pair<double, double> PoldiDetectorDecorator::qLimits(double lambdaMin,
                                                          double lambdaMax) {
  if (m_decoratedDetector) {
    return m_decoratedDetector->qLimits(lambdaMin, lambdaMax);
  } else {
    throw std::runtime_error("No detector decorated!");
  }
}

void PoldiDetectorDecorator::detectorSetHook() {}

} // namespace Poldi
} // namespace Mantid
