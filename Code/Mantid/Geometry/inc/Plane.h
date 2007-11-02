#ifndef Plane_h
#define Plane_h

namespace Mantid
{

namespace Geometry
{

/*!
  \class Plane
  \brief Holds a simple Plane
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a plane and a normal and a distance from
  the origin
*/
class Plane : public Surface
{
 private:

  static Logger& PLog;           ///< The official logger

  const double PTolerance;      ///< Tolerance to the surfaces.

  Geometry::Vec3D NormV;         ///< Normal vector
  double Dist;         ///< Distance 

  int planeType() const;         ///< are we alined on an axis

 public:

  /// Effective typename 
  virtual std::string className() const { return "Plane"; }

  static int possibleLine(const std::string&);

  Plane();
  Plane(const Plane&);
  Plane* clone() const;
  Plane& operator=(const Plane&);
  ~Plane();
  
  int setPlane(const Geometry::Vec3D&,const Geometry::Vec3D&);
  int setPlane(const std::string&);
  int side(const Geometry::Vec3D&) const;
  int onSurface(const Geometry::Vec3D&) const;
  // stuff for finding intersections etc.
  double dotProd(const Plane&) const;      ///< returns normal dot product
  Geometry::Vec3D crossProd(const Plane&) const;      ///< returns normal cross product
  double distance(const Geometry::Vec3D&) const;      ///< distance from a point

  double getDistance() const { return Dist; }  ///< Distance from origin
  Geometry::Vec3D getNormal() const { return NormV; }    ///< Normal to plane (+ve surface)

  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

  int setSurface(const std::string&);
  void print() const;
  void write(std::ostream&) const;        ///< Write in MCNPX form

  void setBaseEqn() ;                      ///< set up to be eqn based

  void procXML(XML::XMLcollect&) const;
  int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,
		const int =0);

};

} // NAMESPACE MonteCarlo
}  // NAMESPACE Mantid

#endif
