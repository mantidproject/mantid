#ifndef MANTID_GEOMETRY_OBJCOMPONENT_H_
#define MANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Material.h"
#include "MantidGeometry/Objects/Object.h"

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
class MANTID_GEOMETRY_DLL ObjComponent : public virtual IObjComponent, public Component
{
public:
  ///type string
  virtual std::string type() const {return "PhysicalComponent";}

  /// Constructor for parametrized component
  ObjComponent(const IComponent* base, const ParameterMap * map);
  // Looking to get rid of the first of these constructors in due course (and probably add others)
  explicit ObjComponent(const std::string& name, IComponent* parent=0);
  explicit ObjComponent(const std::string& name, Object_const_sptr shape, IComponent* parent=0,
                        Kernel::Material_sptr material = Kernel::Material_sptr());
  virtual ~ObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const {return new ObjComponent(*this);}

  bool isValid(const Kernel::V3D& point) const;
  bool isOnSide(const Kernel::V3D& point) const;
  int interceptSurface(Track& track) const;
  double solidAngle(const Kernel::V3D& observer) const;
  ///@todo This should go in favour of just the class related one.
  void boundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
  /// get bounding box, which may or may not be axis aligned;
  void getBoundingBox(BoundingBox& absoluteBB) const;  
  /// get Height (Y-dimension) value for component
  virtual double getHeight() const;
  /// get Width (X-dimension) value for component
  virtual double getWidth() const;
  /// get Depth (Z-dimension) value for component
  virtual double getDepth() const;
  
  int getPointInObject(Kernel::V3D& point) const;
  //Rendering member functions
  void draw() const;
  void drawObject() const;
  void initDraw() const;

  /// Return the shape of the component
  const Object_const_sptr shape() const;
  /// Set a new shape on the component
  void setShape(Object_const_sptr newShape);
  /// Return the material this component is made from
  const Kernel::Material_const_sptr material() const;

protected:
  /// The physical geometry representation
  // Made a pointer to a const object. Since this is a shared object we shouldn't be
  // exposing non-const methods of Object through this class.
  Object_const_sptr m_shape;
  /// The material this object is made of
  Kernel::Material_const_sptr m_material;

  const Kernel::V3D factorOutComponentPosition(const Kernel::V3D& point) const;
  const Kernel::V3D takeOutRotation(Kernel::V3D point) const;
private:
/// common part of the two Bounding box functions above;
  void getRelativeBoundingBox(BoundingBox& RelativeBB) const;

};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_OBJCOMPONENT_H_*/
