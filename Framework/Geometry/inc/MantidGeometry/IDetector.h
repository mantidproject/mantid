// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/IObjComponent.h"

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class V3D;
} // namespace Kernel

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
*/
class MANTID_GEOMETRY_DLL IDetector : public virtual IObjComponent {
public:
  /// Create a cloned instance with a parameter map applied
  virtual IDetector *cloneParameterized(const ParameterMap *map) const = 0;

  /// Get the detector ID
  virtual detid_t getID() const = 0;

  /// Get the number of physical detectors this object represents
  virtual std::size_t nDets() const = 0;

  /** Get the distance of this detector object from another Component
   *  @param comp :: The component to give the distance to
   *  @return The distance
   */
  double getDistance(const IComponent &comp) const override = 0;

  /** Gives the angle of this detector object with respect to an axis
   *  @param observer :: The point to calculate the angle relative to (typically
   * the sample position)
   *  @param axis ::     The axis to which the required angle is relative
   *  @return The angle in radians
   */
  virtual double getTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis) const = 0;

  /** Gives the signed angle of this detector object with respect to an axis
   *  @param observer :: The point to calculate the angle relative to (typically
   * the sample position)
   *  @param axis ::     The axis to which the required angle is relative
   *  @param instrumentUp :: Direction corresponding to the instrument up
   * direction. Used to determine signs.
   *  @return The angle in radians
   */
  virtual double getSignedTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis,
                                   const Kernel::V3D &instrumentUp) const = 0;

  /// Gives the phi of this detector object in radians
  virtual double getPhi() const = 0;

  /// Gives the phi of this detector offset from y=0 by offset.
  virtual double getPhiOffset(const double &offset) const = 0;

  /// returns the geometry of detectors, meaningful for groups, rectangular for
  /// single; returns the centre of a detector
  virtual det_topology getTopology(Kernel::V3D &center) const = 0;

  /// Helper for legacy access mode. Returns a reference to the ParameterMap.
  virtual const ParameterMap &parameterMap() const = 0;
  /// Helper for legacy access mode. Returns the index of the detector.
  virtual size_t index() const = 0;

  /// (Empty) Constructor.
  /// prevent Warning C4436 and many failing unit tests on MSVC 2015.
  IDetector() {} // NOLINT
};

/// Shared pointer to IDetector
using IDetector_sptr = std::shared_ptr<Mantid::Geometry::IDetector>;
/// Shared pointer to IDetector (const version)
using IDetector_const_sptr = std::shared_ptr<const Mantid::Geometry::IDetector>;

} // namespace Geometry
} // namespace Mantid
