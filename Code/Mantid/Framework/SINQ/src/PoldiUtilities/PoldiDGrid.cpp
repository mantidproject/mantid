#include "MantidSINQ/PoldiUtilities/PoldiDGrid.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"

namespace Mantid {
namespace Poldi {

PoldiDGrid::PoldiDGrid(boost::shared_ptr<PoldiAbstractDetector> detector,
                       boost::shared_ptr<PoldiAbstractChopper> chopper,
                       double deltaT, std::pair<double, double> wavelengthRange)
    : m_detector(detector), m_chopper(chopper), m_deltaT(deltaT),
      m_wavelengthRange(wavelengthRange), m_dRangeAsMultiples(), m_deltaD(0.0),
      m_dgrid(), m_hasCachedCalculation(false) {}

void
PoldiDGrid::setDetector(boost::shared_ptr<PoldiAbstractDetector> newDetector) {
  m_detector = newDetector;
}

void
PoldiDGrid::setChopper(boost::shared_ptr<PoldiAbstractChopper> newChopper) {
  m_chopper = newChopper;
}

void PoldiDGrid::setDeltaT(double newDeltaT) { m_deltaT = newDeltaT; }

void PoldiDGrid::setWavelengthRange(std::pair<double, double> wavelengthRange) {
  m_wavelengthRange = wavelengthRange;
}

double PoldiDGrid::deltaD() {
  if (!m_hasCachedCalculation) {
    createGrid();
  }

  return m_deltaD;
}

std::vector<double> PoldiDGrid::grid() {
  if (!m_hasCachedCalculation) {
    createGrid();
  }

  return m_dgrid;
}

/** Computes a d-range from the limits set by the detector and expresses it as
  *multiples of a step size given in Angstrom.
  *
  * @return Pair of integers containing the lower and upper d-limit, divided by
  *deltaD.
  */
std::pair<int, int> PoldiDGrid::calculateDRange() {
  std::pair<double, double> qLimits =
      m_detector->qLimits(m_wavelengthRange.first, m_wavelengthRange.second);

  return std::make_pair(
      static_cast<int>(Conversions::qToD(qLimits.second) / m_deltaD),
      static_cast<int>(Conversions::qToD(qLimits.first) / m_deltaD));
}

/** Computes the resolution limit of the POLDI experiment defined by the current
  *instrument, in Angstrom, given the size of one time bin.
  *
  * Since this calculation is based on the time of flight, this value may be
  *different for each point
  * of the detector, depending on the geometry. In the current implementation
  *this is not taken into account,
  * instead the value for the center of the detector is calculated and assumed
  *constant for the whole detector.
  *
  * @return Resolution in Angstrom corresponding to
  */
double PoldiDGrid::calculateDeltaD() {
  int centralElement = static_cast<int>(m_detector->centralElement());

  return Conversions::TOFtoD(m_deltaT,
                             m_chopper->distanceFromSample() +
                                 m_detector->distanceFromSample(centralElement),
                             sin(m_detector->twoTheta(centralElement) / 2.0));
}

/** Generates an equidistant grid of d-values with a given step size. The result
  *depends on the assigned detector, chopper, wavelength and timing.
  *
  */
void PoldiDGrid::createGrid() {
  if (!m_detector) {
    throw std::runtime_error(
        "PoldiDGrid cannot operate with an invalid detector.");
  }

  if (!m_chopper) {
    throw std::runtime_error(
        "PoldiDGrid cannot operate with an invalid chopper.");
  }

  if (m_deltaT <= 0.0) {
    throw std::runtime_error(
        "PoldiDGrid can only operate with positive non-zero time differences");
  }

  if (m_wavelengthRange.first <= 0.0 || m_wavelengthRange.second <= 0.0 ||
      m_wavelengthRange.first >= m_wavelengthRange.second) {
    throw std::runtime_error(
        "PoldiDGrid cannot operate with supplied wavelength range");
  }

  m_deltaD = calculateDeltaD();
  m_dRangeAsMultiples = calculateDRange();

  int ndSpace = m_dRangeAsMultiples.second - m_dRangeAsMultiples.first;
  m_dgrid.resize(ndSpace);
  double d0 = static_cast<double>(m_dRangeAsMultiples.first) * m_deltaD;

  for (int i = 1; i <= ndSpace; ++i) {
    m_dgrid[i - 1] = static_cast<double>(i) * m_deltaD + d0;
  }

  m_hasCachedCalculation = true;
}
}
}
