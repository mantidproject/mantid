#ifndef MANTID_GEOMETRY_DETECTOR_H_
#define MANTID_GEOMETRY_DETECTOR_H_

#include "IDetector.h"
#include "ObjComponent.h"
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

  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
	Detector();
	Detector(const std::string&, Component* reference=0);
	virtual ~Detector();
	virtual Component* clone() const {return new Detector(*this);}
	void setID(int);
	int getID() const;
	
	// IDetector methods
	double getDistance(const Component& comp) const;
  double getAzimuth(const Component& comp) const;

private:
	/// The detector id
	int id;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTOR_H_*/
