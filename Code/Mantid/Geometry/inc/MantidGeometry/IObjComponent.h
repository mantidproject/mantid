#ifndef IMANTID_GEOMETRY_OBJCOMPONENT_H_
#define IMANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/Track.h"
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

    Notably, this contains a GeometryHandler and methods used to render the component in
    the instrument 3D view.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  IObjComponent(GeometryHandler* the_handler);

  // Looking to get rid of the first of these constructors in due course (and probably add others)
  virtual ~IObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const = 0;

  /// Does the point given lie within this object component?
  virtual bool isValid(const V3D& point) const = 0;

  /// Does the point given lie on the surface of this object component?
  virtual bool isOnSide(const V3D& point) const = 0;

  ///Checks whether the track given will pass through this Component.
  virtual int interceptSurface(Track& track) const = 0;

  /// Finds the approximate solid angle covered by the component when viewed from the point given
  virtual double solidAngle(const V3D& observer) const = 0;

  /**
   * Given an input estimate of the axis aligned (AA) bounding box (BB), return an improved set of values.
   * The AA BB is determined in the frame of the object and the initial estimate will be transformed there.
   * The returned BB will be the frame of the ObjComponent and may not be optimal.
   * Takes input axis aligned bounding box max and min points and calculates the bounding box for the
   * object and returns them back in max and min points. Cached values used after first call.
   * @param xmax :: Maximum value for the bounding box in x direction
   * @param ymax :: Maximum value for the bounding box in y direction
   * @param zmax :: Maximum value for the bounding box in z direction
   * @param xmin :: Minimum value for the bounding box in x direction
   * @param ymin :: Minimum value for the bounding box in y direction
   * @param zmin :: Minimum value for the bounding box in z direction
   */
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const = 0;
  
  ///Try to find a point that lies within (or on) the object
  virtual int getPointInObject(V3D& point) const = 0;

  //Rendering member functions
  ///Draws the objcomponent.
  virtual void draw() const = 0;

  /// Draws the Object.
  virtual void drawObject() const = 0;

  /// Initializes the ObjComponent for rendering, this function should be called before rendering.
  virtual void initDraw() const = 0;

  /// Returns the shape of the Object
  virtual const boost::shared_ptr<const Object> Shape()const = 0;

  void setScaleFactor(double xFactor,double yFactor, double zFactor);

  ///Gets the scaling factor of the object for the Object Component.
  V3D  getScaleFactor(){return m_ScaleFactor;}

  /// Gets the GeometryHandler
  GeometryHandler* Handle()const{return handle;}

protected:
  /// Reset the current geometry handler
  void setGeometryHandler(GeometryHandler *h);
  /// Object Scaling factor in 3 axis direction. given as a vector
  V3D	m_ScaleFactor;
  
private:
    /// Protected copy constructor
  IObjComponent(const IObjComponent&);
  /// Private, unimplemented copy assignment operator
  IObjComponent& operator=(const IObjComponent&);
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
