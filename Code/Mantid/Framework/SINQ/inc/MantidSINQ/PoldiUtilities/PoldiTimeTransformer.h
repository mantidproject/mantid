#ifndef MANTID_SINQ_POLDITIMETRANSFORMER_H_
#define MANTID_SINQ_POLDITIMETRANSFORMER_H_

#include "MantidKernel/System.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"

namespace Mantid {
namespace Poldi {

/** PoldiTimeTransformer

    This helper class transforms peaks from "d" to "time", using
    factors derived from POLDI detector configuration.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 22/05/2014

      Copyright © 2014 PSI-MSS

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

struct DetectorElementCharacteristics {
  DetectorElementCharacteristics()
      : distance(0.0), totalDistance(0.0), twoTheta(0.0), sinTheta(0.0),
        cosTheta(1.0), tof1A(0.0) {}

  DetectorElementCharacteristics(int element,
                                 const PoldiAbstractDetector_sptr &detector,
                                 const PoldiAbstractChopper_sptr &chopper) {
    distance = detector->distanceFromSample(element);
    totalDistance =
        detector->distanceFromSample(element) + chopper->distanceFromSample();
    twoTheta = detector->twoTheta(element);
    sinTheta = sin(twoTheta / 2.0);
    cosTheta = cos(twoTheta / 2.0);
    tof1A = Conversions::dtoTOF(1.0, totalDistance, sinTheta);
  }

  double distance;
  double totalDistance;
  double twoTheta;
  double sinTheta;
  double cosTheta;
  double tof1A;
};

class DetectorElementData {
public:
  DetectorElementData(int element, const DetectorElementCharacteristics &center,
                      const PoldiAbstractDetector_sptr &detector,
                      const PoldiAbstractChopper_sptr &chopper) {
    DetectorElementCharacteristics current(element, detector, chopper);

    m_intensityFactor = pow(center.distance / current.distance, 2.0) *
                        current.sinTheta / center.sinTheta;
    m_lambdaFactor = 2.0 * current.sinTheta / center.tof1A;
    m_timeFactor = current.sinTheta / center.sinTheta * current.totalDistance /
                   center.totalDistance;
    m_widthFactor = current.cosTheta - center.cosTheta;
    m_tofFactor = center.tof1A / current.tof1A;
  }

  double intensityFactor() const { return m_intensityFactor; }
  double lambdaFactor() const { return m_lambdaFactor; }
  double timeFactor() const { return m_timeFactor; }
  double widthFactor() const { return m_widthFactor; }
  double tofFactor() const { return m_tofFactor; }

protected:
  double m_intensityFactor;
  double m_lambdaFactor;
  double m_timeFactor;
  double m_widthFactor;
  double m_tofFactor;
};

typedef boost::shared_ptr<const DetectorElementData>
    DetectorElementData_const_sptr;

class MANTID_SINQ_DLL PoldiTimeTransformer {
public:
  PoldiTimeTransformer();
  PoldiTimeTransformer(const PoldiInstrumentAdapter_sptr &poldiInstrument);
  virtual ~PoldiTimeTransformer() {}

  void initializeFromPoldiInstrument(
      const PoldiInstrumentAdapter_sptr &poldiInstrument);

  size_t detectorElementCount() const;

  double dToTOF(double d) const;
  double detectorElementIntensity(double centreD, size_t detectorIndex) const;

  double calculatedTotalIntensity(double centreD) const;

protected:
  std::vector<DetectorElementData_const_sptr>
  getDetectorElementData(const PoldiAbstractDetector_sptr &detector,
                         const PoldiAbstractChopper_sptr &chopper);
  DetectorElementCharacteristics
  getDetectorCenterCharacteristics(const PoldiAbstractDetector_sptr &detector,
                                   const PoldiAbstractChopper_sptr &chopper);

  DetectorElementCharacteristics m_detectorCenter;
  std::vector<DetectorElementData_const_sptr> m_detectorElementData;
  double m_detectorEfficiency;
  size_t m_chopperSlits;

  PoldiSourceSpectrum_const_sptr m_spectrum;
};

typedef boost::shared_ptr<PoldiTimeTransformer> PoldiTimeTransformer_sptr;

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDITIMETRANSFORMER_H_ */
