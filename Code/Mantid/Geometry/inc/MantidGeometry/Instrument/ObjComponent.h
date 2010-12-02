#ifndef MANTID_GEOMETRY_OBJCOMPONENT_H_
#define MANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Objects/Material.h"
#include "MantidGeometry/Objects/Object.h"
#include "boost/shared_ptr.hpp"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace Geometry
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------

/** Object Component class, this class brings together the physical attributes of the component
    to the positioning and geometry tree.

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
class DLLExport ObjComponent : public virtual IObjComponent, public Component
{
public:
  ///type string
  virtual std::string type() {return "PhysicalComponent";}

  /// Constructor for parametrized component
  ObjComponent(const IComponent* base, const ParameterMap * map);
  // Looking to get rid of the first of these constructors in due course (and probably add others)
  explicit ObjComponent(const std::string& name, IComponent* parent=0);
  explicit ObjComponent(const std::string& name, Object_sptr shape, IComponent* parent=0,
			Material_sptr material = Material_sptr());
  virtual ~ObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const {return new ObjComponent(*this);}

  bool isValid(const V3D& point) const;
  bool isOnSide(const V3D& point) const;
  int interceptSurface(Track& track) const;
  double solidAngle(const V3D& observer) const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
  void getBoundingBox(BoundingBox& absoluteBB) const;
  int getPointInObject(V3D& point) const;
  //Rendering member functions
  void draw() const;
  void drawObject() const;
  void initDraw() const;
  V3D getScaleFactorP() const;

  ///Return the shape of the component
  const boost::shared_ptr<const Object> shape()const;
  /// Return the material this component is made from
  const Material_const_sptr material() const;

protected:
  ObjComponent(const ObjComponent&);

  /// The phyical geometry representation
  // Made a const pointer to a const object. Since this is a shared object we shouldn't be
  // exposing non-const methods of Object through this class.
  Object_const_sptr m_shape;
  /// The material this object is made of
  Material_const_sptr m_material;

private:
  /// Private, unimplemented copy assignment operator
  ObjComponent& operator=(const ObjComponent&);

  const V3D factorOutComponentPosition(const V3D& point) const;
  const V3D takeOutRotation(V3D point) const;

};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_OBJCOMPONENT_H_*/
