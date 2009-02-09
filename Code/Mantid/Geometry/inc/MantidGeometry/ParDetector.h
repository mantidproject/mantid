#ifndef MANTID_GEOMETRY_PARDETECTOR_H_
#define MANTID_GEOMETRY_PARDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/ParObjComponent.h"
#include <string>

namespace Mantid
{
namespace Geometry
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Detector;

/** An extension of the ObjectComponent class to add a detector id.

  @author Roman Tolchenov, Tessella Support Services plc

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
class DLLExport ParDetector : public ParObjComponent, public IDetector
{
public:
  ///A string representation of the component type
	virtual std::string type() const {return "ParDetectorComponent";}
	ParDetector(const Detector* base, const ParameterMap* map);
	virtual ~ParDetector();
	virtual IComponent* clone() const {return new ParDetector(*this);}
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
  ParDetector(const ParDetector&);

private:
  /// Private, unimplemented copy assignment operator
  ParDetector& operator=(const ParDetector&);

};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARDETECTOR_H_*/
