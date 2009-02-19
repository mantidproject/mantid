#ifndef IMANTID_GEOMETRY_OBJCOMPONENT_H_
#define IMANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Track.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{
namespace Geometry
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class Object;
class GeometryHandler;

/** Object Component class, this class brings together the physical attributes of the component
    to the positioning and geometry tree.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IObjComponent : public virtual IComponent
{
public:
  ///type string
  virtual std::string type() {return "IObjComponent";}

  IObjComponent();
  // Looking to get rid of the first of these constructors in due course (and probably add others)
  virtual ~IObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const = 0;
  virtual bool isValid(const V3D& point) const = 0;
  virtual bool isOnSide(const V3D& point) const = 0;
  virtual int interceptSurface(Track& track) const = 0;
  virtual double solidAngle(const V3D& observer) const = 0;
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const = 0;
  virtual int getPointInObject(V3D& point) const = 0;
  //Rendering member functions
  virtual void draw() const = 0;
  virtual void drawObject() const = 0;
  virtual void initDraw() const = 0;
  virtual const boost::shared_ptr<const Object> Shape()const = 0;
  void setScaleFactor(double xFactor,double yFactor, double zFactor);
  V3D  getScaleFactor(){return m_ScaleFactor;}
  GeometryHandler* Handle()const{return handle;}

protected:
  IObjComponent(const IObjComponent&);
  /// Object Scaling factor in 3 axis direction. given as a vector
  V3D	m_ScaleFactor;

private:
  /// Private, unimplemented copy assignment operator
  IObjComponent& operator=(const IObjComponent&);
  /// Geometry Handle for rendering
  GeometryHandler* handle;

  /**
  * Set the geometry handler for IObjComponent
  * @param[in] handle is pointer to the geometry handler. don't delete this pointer in the calling function.
  */
  void setGeometryHandler(GeometryHandler *h)
  {
      if(handle==NULL)return;
      this->handle=handle;
  }

  friend class GeometryHandler;
};

/// Shared pointer to IInstrument
typedef boost::shared_ptr<IObjComponent> IObjComponent_sptr;
/// Shared pointer to IInstrument (const version)
typedef const boost::shared_ptr<const IObjComponent> IObjComponent_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_OBJCOMPONENT_H_*/
