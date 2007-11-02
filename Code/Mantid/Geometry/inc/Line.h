#ifndef Line_h
#define Line_h

namespace Mantid
{

namespace Geometry
{

/*!
  \class Line
  \brief Impliments a line
  \author S. Ansell
  \date Apr 2005
  \version 0.7
  
  Impliments the line 
  \f[ r=\vec{O} + \lambda \vec{n} \f]
*/

class Surface;
class Plane;
class Cylinder;
class Cone;
class Sphere;
class General;
  
class Line 
{
  
 private:

  static Logger& PLog;           ///< The official logger


  Geometry::Vec3D Origin;   ///< Orign point (on plane)
  Geometry::Vec3D Direct;   ///< Direction of outer surface (Unit Vector) 
  
  int lambdaPair(const int,const std::pair<std::complex<double>,
		 std::complex<double> >&,std::vector<Geometry::Vec3D>&) const;

 public: 

  Line();
  Line(const Geometry::Vec3D&,const Geometry::Vec3D&);
  Line(const Line&);
  Line& operator=(const Line&);
  
  virtual ~Line();

  Geometry::Vec3D getPoint(const double) const;   ///< gets the point O+lam*N
  Geometry::Vec3D getOrigin() const { return Origin; }   ///< returns the origin
  Geometry::Vec3D getDirect() const { return Direct; }   ///< returns the direction
  double distance(const Geometry::Vec3D&) const;  ///< distance from line
  int isValid(const Geometry::Vec3D&) const;     ///< Is the point on the line
  void print() const;

  void rotate(const Geometry::Matrix<double>&);
  void displace(const Geometry::Vec3D&);

  int setLine(const Geometry::Vec3D&,const Geometry::Vec3D&);     ///< input Origin + direction

  int intersect(std::vector<Geometry::Vec3D>&,const Surface&) const;
  int intersect(std::vector<Geometry::Vec3D>&,const Cylinder&) const;
  int intersect(std::vector<Geometry::Vec3D>&,const Plane&) const;
  int intersect(std::vector<Geometry::Vec3D>&,const Sphere&) const;

  int intersect(std::vector<Geometry::Vec3D>&,const Line*) const;
  //  int intersect(std::vector<Geometry::Vec3D>&,const Circle*) const;
  
};

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
