#ifndef VisitBase_h
#define VisitBase_h

namespace Mantid
{

namespace Visitors
{

class Surface;
class Plane;
class Cylinder;
class Cone;
class Sphere;
class General;


class BaseVisit
{
public:

  virtual void Accept(const Surface&) =0;
  virtual void Accept(const Plane&) =0;
  virtual void Accept(const Cone&) =0;
  virtual void Accept(const Cylinder&) =0;
  virtual void Accept(const General&) =0;

};

}

}

#endif
