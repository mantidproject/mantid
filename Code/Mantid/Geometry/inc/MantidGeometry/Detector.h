#ifndef MANTID_GEOMETRY_DETECTOR_H_
#define MANTID_GEOMETRY_DETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/ObjComponent.h"
#include <string>

namespace Mantid
{
namespace Geometry
{

/** An extension of the ObjectComponent class to add a detector id.

  @class Detector
  @version A
  @author Laurent C Chapon, ISIS RAL
  @date 01/11/2007

  Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
class DLLExport Detector : public ObjComponent, public IDetector
{
public:
  ///A string representation of the component type
	virtual std::string type() const {return "DetectorComponent";}
	Detector(const std::string& name, Component* parent);
  Detector(const std::string& name, boost::shared_ptr<Object> shape, Component* parent);
	virtual ~Detector();
	virtual Component* clone() const {return new Detector(*this);}
	void setID(int);

	// IDetector methods
  int getID() const;
	V3D getPos() const;
	double getDistance(const IComponent& comp) const;
  double getTwoTheta(const V3D& observer, const V3D& axis) const;
  double solidAngle(const V3D& observer) const;	
	bool isDead() const;
	void markDead();
	bool isMonitor() const;
	// end IDetector methods

	void markAsMonitor(const bool flag = true);

protected:
  Detector(const Detector&);

private:
  /// Private, unimplemented copy assignment operator
  Detector& operator=(const Detector&);

	/// The detector id
	int m_id;
	/// Flags if the detector is dead
	bool m_isDead;
	/// Flags if this is a monitor
	bool m_isMonitor;

	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTOR_H_*/
