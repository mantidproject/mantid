#ifndef MANTID_GEOMETRY_IDETECTOR_H_
#define MANTID_GEOMETRY_IDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IObjComponent.h"
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <map>

namespace Mantid
{

  /// Typedef for the ID of a detector
  typedef int32_t detid_t;

#ifndef HAS_UNORDERED_MAP_H
  /// Map with key = spectrum number, value = workspace index
  typedef std::map<detid_t, size_t> detid2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::map<size_t, detid_t> index2detid_map;
#else
  /// Map with key = spectrum number, value = workspace index
  typedef std::tr1::unordered_map<detid_t, size_t> detid2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::tr1::unordered_map<size_t, detid_t> index2detid_map;
#endif

namespace Geometry
{

  //----------------------------------------------------------------------
  // Forward declaration
  //----------------------------------------------------------------------
  class V3D;

/** Interface class for detector objects.

    @author Russell Taylor, Tessella Support Services plc
    @date 08/04/2008

    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IDetector : public virtual IObjComponent
{
public:
  /// Get the detector ID
  virtual detid_t getID() const = 0;

  /// Get the number of physical detectors this object represents
  virtual std::size_t nDets() const = 0;

  /** Get the distance of this detector object from another Component
   *  @param comp :: The component to give the distance to
   *  @return The distance
   */
  virtual double getDistance(const IComponent& comp) const = 0;

  /** Gives the angle of this detector object with respect to an axis
   *  @param observer :: The point to calculate the angle relative to (typically the sample position)
   *  @param axis ::     The axis to which the required angle is relative
   *  @return The angle in radians
   */
  virtual double getTwoTheta(const V3D& observer, const V3D& axis) const = 0;

  /// Gives the phi of this detector object in radians
  virtual double getPhi() const = 0;

  /// Indicates whether the detector has been masked
  virtual bool isMasked() const = 0;

  /// Indicates whether this is a monitor detector
  virtual bool isMonitor() const = 0;

  /// Get Nearest Neighbours
  virtual std::map<detid_t, double> getNeighbours(double radius = 0.0) = 0;

  /// Must return a pointer to itself if derived from IComponent
  virtual IComponent* getComponent();
  /// (Empty) Constructor
  IDetector() {}
  /// Virtual destructor
  virtual ~IDetector() {}

};

/// Shared pointer to IDetector
typedef boost::shared_ptr<Mantid::Geometry::IDetector> IDetector_sptr;
/// Shared pointer to IDetector (const version)
typedef boost::shared_ptr<const Mantid::Geometry::IDetector> IDetector_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_IDETECTOR_H_*/
