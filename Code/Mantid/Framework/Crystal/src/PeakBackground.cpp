#include "MantidCrystal/PeakBackground.h"
#include "MantidAPI/IPeak.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::API::IPeak;

namespace Mantid {
namespace Crystal {

//----------------------------------------------------------------------------------------------
/** Constructor
*/
PeakBackground::PeakBackground(IPeaksWorkspace_const_sptr peaksWS,
                               const double &radiusEstimate,
                               const double &thresholdSignal,
                               const Mantid::API::MDNormalization normalisation,
                               const SpecialCoordinateSystem coordinates)
    : HardThresholdBackground(thresholdSignal, normalisation),
      m_peaksWS(peaksWS), m_radiusEstimate(radiusEstimate),
      m_mdCoordinates(coordinates) {

  if (m_mdCoordinates == QLab) {
    m_coordFunction = &IPeak::getQLabFrame;
  } else if (m_mdCoordinates == QSample) {
    m_coordFunction = &IPeak::getQSampleFrame;
  } else if (m_mdCoordinates == Mantid::API::HKL) {
    m_coordFunction = &IPeak::getHKL;
  } else {
    throw std::invalid_argument(
        "Unknown CoordinateSystem provided to PeakBackground");
  }
}

PeakBackground::PeakBackground(const PeakBackground &other)
    : HardThresholdBackground(other), m_peaksWS(other.m_peaksWS),
      m_radiusEstimate(other.m_radiusEstimate),
      m_mdCoordinates(other.m_mdCoordinates),
      m_coordFunction(other.m_coordFunction) {}

PeakBackground &PeakBackground::operator=(const PeakBackground &other) {
  if (this != &other) {
    HardThresholdBackground::operator=(other);
    m_peaksWS = other.m_peaksWS;
    m_radiusEstimate = other.m_radiusEstimate;
    m_mdCoordinates = other.m_mdCoordinates;
    m_coordFunction = other.m_coordFunction;
  }
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
PeakBackground::~PeakBackground() {}

/// Virutal constructor
PeakBackground *PeakBackground::clone() const {
  return new PeakBackground(*this);
}

bool PeakBackground::isBackground(Mantid::API::IMDIterator *iterator) const {
  if (!HardThresholdBackground::isBackground(iterator)) {
    const VMD &center = iterator->getCenter();
    V3D temp(center[0], center[1], center[2]); // This assumes dims 1, 2, and 3
                                               // in the workspace correspond to
                                               // positions.

    for (int i = 0; i < m_peaksWS->getNumberPeaks(); ++i) {
      const IPeak &peak = m_peaksWS->getPeak(i);
      V3D coords = m_coordFunction(&peak);
      if (coords.distance(temp) < m_radiusEstimate) {
        return false;
      }
    }
  }
  return true;
}

void PeakBackground::configureIterator(Mantid::API::IMDIterator *const) const {}

} // namespace Crystal
} // namespace Mantid