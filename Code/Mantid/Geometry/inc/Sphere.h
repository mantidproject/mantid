#ifndef Sphere_h
#define Sphere_h

namespace Geometry
{

/*!
  \class Sphere
  \brief Holds a Sphere as vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a sphere as a centre point and a radius.
*/

class Sphere : public Surface
{
 private:
  
  Geometry::Vec3D Centre;        ///< Point for centre
  double Radius;                 ///< Radius of sphere
  
  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

 public:
  
  Sphere();
  Sphere(const Sphere&);
  Sphere* clone() const;
  Sphere& operator=(const Sphere&);
  ~Sphere();
  
  int setSurface(const std::string&);
  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;
  double distance(const Geometry::Vec3D&) const;

  void setCentre(const Geometry::Vec3D&);              
  Geometry::Vec3D getCentre() const { return Centre; } ///< Get Centre
  double getRadius() const { return Radius; } ///< Get Radius
  void setBaseEqn();

  void write(std::ostream&) const; 
  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,const int=0);

};

}
 
#endif
