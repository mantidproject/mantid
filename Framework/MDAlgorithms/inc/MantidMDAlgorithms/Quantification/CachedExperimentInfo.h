// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_
#define MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {
class Goniometer;
}
namespace MDAlgorithms {
/**
 * Defines information about a neutron event within a given experiment
 * that was detected by a detector with a given ID.
 *
 * It also serves as a cache for storing quicker lookups to frequently
 * used distances and values, i.e twoTheta, phi etc.
 */
class DLLExport CachedExperimentInfo {
public:
  /// Constructor
  CachedExperimentInfo(const API::ExperimentInfo &exptInfo,
                       const detid_t detID);

  /// Disable default constructor
  CachedExperimentInfo() = delete;

  /// Disable copy operator
  CachedExperimentInfo(const CachedExperimentInfo &) = delete;

  /// Disable assignment operator
  CachedExperimentInfo &operator=(const CachedExperimentInfo &) = delete;

  /// Return the experiment info
  inline const API::ExperimentInfo &experimentInfo() const {
    return m_exptInfo;
  }
  /// Returns the efixed value for this detector/experiment
  double getEFixed() const;
  /// Returns the scattering angle theta in radians
  double twoTheta() const;
  /// Returns the azimuth angle phi in radians
  double phi() const;

  /// Returns the distance from the moderator to the first chopper in metres
  double moderatorToFirstChopperDistance() const;
  /// Returns the distance from the first aperture to the first chopper in
  /// metres
  double firstApertureToFirstChopperDistance() const;
  /// Returns the distance from the chopper to the sample in metres
  double firstChopperToSampleDistance() const;
  /// Returns the sample to detector distance in metres
  double sampleToDetectorDistance() const;

  /// Returns the aperture dimensions
  const std::pair<double, double> &apertureSize() const;
  /// Returns the widths of a cuboid that encloses the sample
  const Kernel::V3D &sampleCuboid() const;
  /// Returns a V3D that defines the detector volume
  const Kernel::V3D detectorVolume() const;

  /// Returns the D Matrix. Converts from lab coordindates -> detector
  /// coordinates
  const Kernel::DblMatrix &labToDetectorTransform() const;
  /// Returns the matrix required to move from sample coordinates -> detector
  /// coordinates
  const Kernel::DblMatrix &sampleToDetectorTransform() const;

private:
  /// Cache frequently used values
  void initCaches(const Geometry::Instrument_const_sptr &instrument,
                  const detid_t detID);

  /// A pointer to the experiment description
  const API::ExperimentInfo &m_exptInfo;

  /// The efixed value
  double m_efixed;
  /// Two theta cache
  double m_twoTheta;
  /// phi cache
  double m_phi;
  /// Source to chopper
  double m_modToChop;
  /// Aperture to chopper
  double m_apertureToChop;
  /// Sample to chopper
  double m_chopToSample;
  /// Sample to detector
  double m_sampleToDet;

  /// Reference axes
  unsigned int m_beam, m_up, m_horiz;
  /// Aperture dimensions
  std::pair<double, double> m_apertureSize;
  /// Sample box size
  Kernel::V3D m_sampleWidths;
  /// Detector's bounding box
  Geometry::BoundingBox m_detBox;
  /// Store the goniometer
  Geometry::Goniometer *m_gonimeter;
  /// Store sample to detector transformation
  Kernel::DblMatrix m_sampleToDetMatrix;
};
} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_ */
