#ifndef MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_
#define MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
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
  DISABLE_DEFAULT_CONSTRUCT(CachedExperimentInfo)
  DISABLE_COPY_AND_ASSIGN(CachedExperimentInfo)

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
}
}

#endif /* MANTID_MDALGORITHMS_CACHEDEXPERIMENTINFO_H_ */
