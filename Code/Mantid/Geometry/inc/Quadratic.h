#ifndef Geometry_Quadratic_h
#define Geometry_Quadratic_h

#include "MantidKernel/System.h"
#include "Surface.h"

namespace Mantid
{

namespace Geometry
{

/*!
  \class  Quadratic
  \brief Holds a basic quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a basic surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  
*/

class DLLExport Quadratic : public Surface
{
 private:
  
  static Kernel::Logger& PLog;           ///< The official logger

  double eqnValue(const Geometry::Vec3D&) const;
  void matrixForm(Geometry::Matrix<double>&,
		  Geometry::Vec3D&,double&) const;          

 protected:

  std::vector<double> BaseEqn;     ///< Base equation (as a 10 point vector)

 public:

  static const int Nprecision=10;        ///< Precision of the output

  Quadratic();
  Quadratic(const Quadratic&);
  virtual Quadratic* clone() const =0;   ///< Abstract clone function
  Quadratic& operator=(const Quadratic&);
  virtual ~Quadratic();

  /// Effective typeid
  virtual std::string className() const { return "Quadratic"; }

  const std::vector<double>& copyBaseEqn() const { return BaseEqn; }  ///< access BaseEquation vector

  virtual int side(const Geometry::Vec3D&) const; 

  virtual void setBaseEqn() =0;      ///< Abstract set baseEqn 

  virtual int onSurface(const Geometry::Vec3D&) const;          ///< is point valid on surface 
  virtual double distance(const Geometry::Vec3D&) const;        ///< distance between point and surface (approx)
  virtual Geometry::Vec3D surfaceNormal(const Geometry::Vec3D&) const;    ///< Normal at surface

  virtual void displace(const Geometry::Vec3D&);
  virtual void rotate(const Geometry::Matrix<double>&);

  virtual void write(std::ostream&) const;
  virtual void print() const;

  virtual void procXML(XML::XMLcollect&) const;
  virtual int importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>&,
			int const=0);
  virtual void writeXML(const std::string&) const;

};

}  // NAMESPACE Geometry

}  // NAMESPACE Geometry

#endif
