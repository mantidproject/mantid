#ifndef MANTID_GEOMETRY_DETECTOR_H_
#define MANTID_GEOMETRY_DETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <string>
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

/**
 * This class represents a detector - i.e. a single pixel in an instrument.
 * It is An extension of the ObjectComponent class to add a detector id.

  @class Detector
  @version A
  @author Laurent C Chapon, ISIS RAL
  @date 01/11/2007

  Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_GEOMETRY_DLL Detector : public virtual IDetector,
                                     public ObjComponent {
public:
  /// A string representation of the component type
  virtual std::string type() const { return "DetectorComponent"; }

  /// Constructor for parametrized version
  Detector(const Detector *base, const ParameterMap *map);
  Detector(const std::string &name, int id, IComponent *parent);
  Detector(const std::string &name, int it, boost::shared_ptr<Object> shape,
           IComponent *parent);
  virtual ~Detector();
  // functions inherited from IObjectComponent
  virtual Component *clone() const { return new Detector(*this); }

  // IDetector methods
  detid_t getID() const;
  std::size_t nDets() const {
    return 1;
  } ///< A Detector object represents a single physical detector
  double getDistance(const IComponent &comp) const;
  double getTwoTheta(const Kernel::V3D &observer,
                     const Kernel::V3D &axis) const;
  double getSignedTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis,
                           const Kernel::V3D &instrumentUp) const;
  double getPhi() const;
  double getPhiOffset(const double &offset) const;
  bool isMasked() const;
  bool isMonitor() const;
  // end IDetector methods
  void markAsMonitor(const bool flag = true);
  /** returns the detector's topology, namely, the meaning of the detector's
     angular measurements.
      It is different in cartesian and cylindrical (surrounding the beam)
     coordinate system */
  det_topology getTopology(Kernel::V3D &center) const;

  /// Return the relative position to the parent
  virtual const Kernel::V3D getRelativePos() const {
    return ObjComponent::getRelativePos();
  }

private:
  /// The detector id
  const detid_t m_id;
  /// Flags if this is a monitor
  bool m_isMonitor;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTOR_H_*/
