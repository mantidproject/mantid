#ifndef MANTID_GEOMETRY_IDETECTOR_H_
#define MANTID_GEOMETRY_IDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class IComponent;
class V3D;

/** Interface class for detector objects.

    @author Russell Taylor, Tessella Support Services plc
    @date 08/04/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IDetector
{
public:
  /// Get the detector ID
  virtual int getID() const = 0;

  /// Get the absolute position of this detector
  virtual V3D getPos() const = 0;

  /** Get the distance of this detector object from another Component
   *  @param comp The component to give the distance to
   *  @return The distance
   */
  virtual double getDistance(const IComponent& comp) const = 0;

  /** Gives the approximate angle subtended by the detector
   *  @param observer The point from which the detector is being viewed
   *  @return The solid angle in steradians
   *  @throw NullPointerException If geometrical form of the detector has not been provided in the instrument definition file
   */
  virtual double solidAngle(const V3D& observer) const = 0;

  /// Indicates whether the detector is to be regarded as dead
  virtual bool isDead() const = 0;
  /// Flag the detector as dead
  virtual void markDead() = 0;

  /// Indicates whether this is a monitor detector
  virtual bool isMonitor() const = 0;

  /// (Empty) Constructor
	IDetector() {}
	/// Virtual destructor
	virtual ~IDetector() {}
};

/// Shared pointer to IDetector
typedef boost::shared_ptr<Mantid::Geometry::IDetector> IDetector_sptr;
/// Shared pointer to IDetector (const version)
typedef const boost::shared_ptr<const Mantid::Geometry::IDetector> IDetector_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_IDETECTOR_H_*/
