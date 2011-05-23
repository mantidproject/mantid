#ifndef MANTID_GEOMETRY_DETECTOR_H_
#define MANTID_GEOMETRY_DETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <string>

namespace Mantid
{
namespace Geometry
{

/**
 * This class represents a detector - i.e. a single pixel in an instrument.
 * It is An extension of the ObjectComponent class to add a detector id.

  @class Detector
  @version A
  @author Laurent C Chapon, ISIS RAL
  @date 01/11/2007

  Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport Detector : public virtual IDetector, public ObjComponent
{
public:
  ///A string representation of the component type
  virtual std::string type() const {return "DetectorComponent";}

  /// Constructor for parametrized version
  Detector(const Detector* base, const ParameterMap * map);
  Detector(const std::string& name, int id, IComponent* parent);
  Detector(const std::string& name, int it, boost::shared_ptr<Object> shape, IComponent* parent);
  virtual ~Detector();
  // functions inherited from IObjectComponent
  virtual Component* clone() const {return new Detector(*this);}

  // IDetector methods
  detid_t getID() const;
  std::size_t nDets() const { return 1; } ///< A Detector object represents a single physical detector
  double getDistance(const IComponent& comp) const;
  double getTwoTheta(const V3D& observer, const V3D& axis) const;
  double getPhi() const;
  bool isMasked() const;
  bool isMonitor() const;
  std::map<int32_t, double> getNeighbours(double radius = 0.0);
  /// Returns a reference to itself
  IComponent* getComponent(){return static_cast<IComponent*>(this);}
  // end IDetector methods

  void markAsMonitor(const bool flag = true);

private:
  /// The detector id
  mutable detid_t m_id;
  /// Flags if this is a monitor
  bool m_isMonitor;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTOR_H_*/
