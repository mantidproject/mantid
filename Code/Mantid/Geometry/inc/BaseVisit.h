#ifndef BaseVisit_h
#define BaseVisit_h

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

  virtual ~BaseVisit() {}

  virtual void Accept(const Surface&) =0;
  virtual void Accept(const Plane&) =0;
  virtual void Accept(const Sphere&) =0;
  virtual void Accept(const Cone&) =0;
  virtual void Accept(const Cylinder&) =0;
  virtual void Accept(const General&) =0;

};


} // NAMESPACE

#endif
