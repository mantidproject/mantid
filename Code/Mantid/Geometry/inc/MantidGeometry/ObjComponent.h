#ifndef OBJCOMPONENT_H_
#define OBJCOMPONENT_H_
#include "MantidKernel/System.h"
#include "MantidGeometry/Component.h"

namespace Mantid
{
namespace Geometry
{
  //forward declaration
	class GeomObj;

  /** @class ObjComponent ObjComponent.h
 	
 	  Object Component class, this class brings together the physical attributes of the component to the positioning and geometry tree.
 			    	
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
 	    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories
 	
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
class DLLExport ObjComponent : public Component
{
public:
  ///type string
	virtual std::string type() {return "PhysicalComponent";}

	ObjComponent();
	ObjComponent(const std::string&, Component* reference=0);
	virtual ~ObjComponent();

  /** Clone function
   * @returns A pointer to a clone of this object
   */
	virtual Component* clone() const {return new ObjComponent(*this);}

  /** Sets the underlying physical geometry object
   * @param o A pointer to the phsyical geometry object
   */
	void setObject(GeomObj* o);
  /** Gets the underlying physical geometry object
   * @returns A pointer to the phsyical geometry object
   */
	const GeomObj* getObject() const;
  /** Gets the level geometry object
   * @returns the level of the  geometry object
   */
	int& getLevel() { return level; }
  /** Gets the level geometry object
   * @returns the level of the  geometry object
   */
	const int& getLevel() const {return level;}
private:
  ///The level of the object in the geometry
	int level;
  /// The phyical geometry representation
	GeomObj* obj;
};

} // Namespace Geometry
} // Namespace Mantid

#endif /*OBJCOMPONENT_H_*/
