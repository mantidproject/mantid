#ifndef VisitBase_h
#define VisitBase_h

namespace Visitors

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

class LineIntersectVisit : public BaseVisit
  {
    public:
    
     
  };
};

class Dispatcher 
{
  virtual void LineDispatch(Cylinder& A,Line& A)
    {
      std::cout<<"LineDispatch for cylinder"
    }
}



class VisitBase
{
 private:
  
 public:
  
  virtual void visit
   
};



#endif
