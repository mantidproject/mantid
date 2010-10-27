#ifndef MANTID_GEOMETRY_PAROBJCOMPONENT_H_
#define MANTID_GEOMETRY_PAROBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Track.h"
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
class Object;
class GeometryHandler;

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
class DLLExport ParObjComponent : public ParametrizedComponent, public virtual IObjComponent
{
public:
  ///type string
  virtual std::string type() {return "ParObjComponent";}

  /// Constructor
  ParObjComponent(const ObjComponent* base, const ParameterMap& map);

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual IComponent* clone() const {return new ParObjComponent(*this);}

  bool isValid(const V3D& point) const;
  bool isOnSide(const V3D& point) const;
  int interceptSurface(Track& track) const;
  double solidAngle(const V3D& observer) const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox& boundingBox) const;
  int getPointInObject(V3D& point) const;
  //Rendering member functions
  void draw() const;
  void drawObject() const;
  void initDraw() const;

protected:
  ParObjComponent(const ParObjComponent&);

  const boost::shared_ptr<const Object> shape()const{return dynamic_cast<const ObjComponent*>(m_base)->shape();}
  //GeometryHandler* Handle()const{return dynamic_cast<const ObjComponent*>(m_base)->Handle();}
  const V3D factorOutComponentPosition(const V3D& point) const;
  const V3D takeOutRotation(V3D point) const;


private:
  /// Private, unimplemented copy assignment operator
  ParObjComponent& operator=(const ParObjComponent&);
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PAROBJCOMPONENT_H_*/
