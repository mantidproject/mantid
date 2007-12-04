#ifndef Cone_h
#define Cone_h

namespace Mantid
{

namespace Geometry 
{

/*!
  \class Cone
  \brief Holds a cone in vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a cone as a centre point (on main axis)
  a vector from that point (unit) and a radius.
  and an angle.
*/


class Cone : public Surface
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger
  
  Geometry::Vec3D Centre;        ///< Geometry::Vec3D for centre
  Geometry::Vec3D Normal;        ///< Normal
  double alpha;                  ///< Angle (degrees)
  double cangle;                 ///< Cos(angle)
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

 public:

  /// Public identifier
  virtual std::string className() const { return "Cone"; }
  
  Cone();
  Cone(const Cone&);
  Cone* clone() const;
  Cone& operator=(const Cone&);
  int operator==(const Cone&) const;
  ~Cone();
  
  int setCone(const std::string&);   ///< Not implemented
  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;

  /// Return centre point
  Geometry::Vec3D getCentre() const { return Centre; }              
  /// Central normal
  Geometry::Vec3D getNormal() const { return Normal; }       
  /// Edge Angle
  double getCosAngle() const { return cangle; } 
  double distance(const Geometry::Vec3D&) const;   

  int setSurface(const std::string&);
  void setCentre(const Geometry::Vec3D&);              
  void setNorm(const Geometry::Vec3D&);       
  void setAngle(const double A);  
  void setTanAngle(const double A);
  void setBaseEqn();

  void write(std::ostream&) const;

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int singleFlag=0);

};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
