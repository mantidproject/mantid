#ifndef Cylinder_h
#define Cylinder_h


namespace Mantid
{

namespace Geometry
{

/*!
  \class  Cylinder
  \brief Holds a cylinder as a vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a cylinder as a centre point (on main axis)
  a vector from that point (unit) and a radius.
*/

class Cylinder : public Surface
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
  Geometry::Vec3D Centre;        ///< Geometry::Vec3D for centre
  Geometry::Vec3D Normal;        ///< Direction of centre line
  int Nvec;            ///< Normal vector is x,y or z :: (1-3) (0 if general)
  double Radius;       ///< Radius of cylinder
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);
  void setNvec();      ///< check to obtain orientation

 public:

  /// Public identifer
  virtual std::string className() const { return "Cylinder"; }  

  
  Cylinder();
  Cylinder(const Cylinder&);
  Cylinder* clone() const;
  Cylinder& operator=(const Cylinder&);
  ~Cylinder();

  // Visit acceptor
  virtual void acceptVisitor(BaseVisit& A) const
    {  A.Accept(*this); }
  
  virtual double lineIntersect(const Geometry::Vec3D&,
			       const Geometry::Vec3D&) const;

  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;
  double distance(const Geometry::Vec3D&) const;

  int setSurface(const std::string&);
  void setCentre(const Geometry::Vec3D&);              
  void setNorm(const Geometry::Vec3D&);       
  Geometry::Vec3D getCentre() const { return Centre; }   ///< Return centre point       
  Geometry::Vec3D getNormal() const { return Normal; }   ///< Return Central line
  double getRadius() const { return Radius; }  ///< Get Radius      
  void setBaseEqn();


  void write(std::ostream&) const;
  void print() const;

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int=0);
  void writeXML(const std::string&);

};

}   // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
