#ifndef MANTID_GEOMETRY_IDETECTOR_H_
#define MANTID_GEOMETRY_IDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDTypes.h"
#include <stdexcept>
#include <map>

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class V3D;
}

namespace Geometry {
/** Describes the topology of a detectors group used to calculate angular
 * position and angular
 *  measures for detectors. The topology defines the meaning of angular
 * measurements for a detector
 */
enum det_topology {
  rect, //< rectangular geometry
  cyl,  //< cylindrical geometry
  undef //< the geometry is yet undefined, if you need to know the geometry, a
  // method to identify it must be deployed
};

/** Interface class for detector objects.

    @author Russell Taylor, Tessella Support Services plc
    @date 08/04/2008

    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL IDetector : public virtual IObjComponent {
public:
  /// Get the detector ID
  virtual detid_t getID() const = 0;

  /// Get the number of physical detectors this object represents
  virtual std::size_t nDets() const = 0;

  /** Get the distance of this detector object from another Component
   *  @param comp :: The component to give the distance to
   *  @return The distance
   */
  virtual double getDistance(const IComponent &comp) const = 0;

  /** Gives the angle of this detector object with respect to an axis
   *  @param observer :: The point to calculate the angle relative to (typically
   * the sample position)
   *  @param axis ::     The axis to which the required angle is relative
   *  @return The angle in radians
   */
  virtual double getTwoTheta(const Kernel::V3D &observer,
                             const Kernel::V3D &axis) const = 0;

  /** Gives the signed angle of this detector object with respect to an axis
   *  @param observer :: The point to calculate the angle relative to (typically
   * the sample position)
   *  @param axis ::     The axis to which the required angle is relative
   *  @param instrumentUp :: Direction corresponding to the instrument up
   * direction. Used to determine signs.
   *  @return The angle in radians
   */
  virtual double getSignedTwoTheta(const Kernel::V3D &observer,
                                   const Kernel::V3D &axis,
                                   const Kernel::V3D &instrumentUp) const = 0;

  /// Gives the phi of this detector object in radians
  virtual double getPhi() const = 0;

  /// Gives the phi of this detector offset from y=0 by offset.
  virtual double getPhiOffset(const double &offset) const = 0;

  /// Indicates whether the detector has been masked
  virtual bool isMasked() const = 0;
  /// Indicates whether this is a monitor detector
  virtual bool isMonitor() const = 0;

  /// returns the geometry of detectors, meaningful for groups, rectangular for
  /// single; returns the centre of a detector
  virtual det_topology getTopology(Kernel::V3D &center) const = 0;

  /// (Empty) Constructor
  IDetector() {}
  /// Virtual destructor
  virtual ~IDetector() {}
};

/// Shared pointer to IDetector
typedef boost::shared_ptr<Mantid::Geometry::IDetector> IDetector_sptr;
/// Shared pointer to IDetector (const version)
typedef boost::shared_ptr<const Mantid::Geometry::IDetector>
    IDetector_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_IDETECTOR_H_*/
