#ifndef IMANTID_GEOMETRY_OBJCOMPONENT_H_
#define IMANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Material.h"

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

    Notably, this contains a GeometryHandler and methods used to render the component in
    the instrument 3D view.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL IObjComponent : public virtual IComponent
{
public:
  ///type string
  virtual std::string type() const {return "IObjComponent";}

  IObjComponent();

  IObjComponent(GeometryHandler* the_handler);

  // Looking to get rid of the first of these constructors in due course (and probably add others)
  virtual ~IObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const = 0;

  /// Does the point given lie within this object component?
  virtual bool isValid(const Kernel::V3D& point) const = 0;

  /// Does the point given lie on the surface of this object component?
  virtual bool isOnSide(const Kernel::V3D& point) const = 0;

  ///Checks whether the track given will pass through this Component.
  virtual int interceptSurface(Track& track) const = 0;

  /// Finds the approximate solid angle covered by the component when viewed from the point given
  virtual double solidAngle(const Kernel::V3D& observer) const = 0;

  ///Try to find a point that lies within (or on) the object
  virtual int getPointInObject(Kernel::V3D& point) const = 0;

  //Rendering member functions
  ///Draws the objcomponent.
  virtual void draw() const = 0;

  /// Draws the Object.
  virtual void drawObject() const = 0;

  /// Initializes the ObjComponent for rendering, this function should be called before rendering.
  virtual void initDraw() const = 0;

  /// Returns the shape of the Object
  virtual const boost::shared_ptr<const Object> shape()const = 0;
  /// Returns the material of the Object
  virtual const boost::shared_ptr<const Kernel::Material> material()const = 0;

  /// Gets the GeometryHandler
  GeometryHandler* Handle()const{return handle;}

protected:
  /// Protected copy constructor
  IObjComponent(const IObjComponent&);
  /// Assignment operato
  IObjComponent& operator=(const IObjComponent&);

  /// Reset the current geometry handler
  void setGeometryHandler(GeometryHandler *h);

private:
  /// Geometry Handle for rendering
  GeometryHandler* handle;

  friend class GeometryHandler;
};

/// Shared pointer to IObjComponent
typedef boost::shared_ptr<IObjComponent> IObjComponent_sptr;
/// Shared pointer to IObjComponent (const version)
typedef boost::shared_ptr<const IObjComponent> IObjComponent_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_OBJCOMPONENT_H_*/
