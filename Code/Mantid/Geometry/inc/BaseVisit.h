#ifndef Geometry_BaseVisit_h
#define Geometry_BaseVisit_h

namespace Mantid
{

namespace Geometry
{

class Surface;
class Plane;
class Cylinder;
class Cone;
class Sphere;
class General;
class Line;

/*!
  \class BaseVisit
  \version 1.0
  \author S. Ansell
  \brief Adds the main
 */

class BaseVisit
{
public:

  /// Destructor
  virtual ~BaseVisit() {}

  virtual void Accept(const Surface&) =0;  ///< Accept a surface
  virtual void Accept(const Plane&) =0;    ///< Accept a plane
  virtual void Accept(const Sphere&) =0;   ///< Accept a sphere
  virtual void Accept(const Cone&) =0;     ///< Accept a cone
  virtual void Accept(const Cylinder&) =0; ///< Accept a cylinder
  virtual void Accept(const General&) =0;  ///< Accept a general surface

};


} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
