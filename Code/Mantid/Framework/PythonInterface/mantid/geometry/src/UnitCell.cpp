#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidPythonInterface/kernel/NumpyConverters.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>

using Mantid::Geometry::UnitCell;
using Mantid::Geometry::AngleUnits;
using Mantid::Geometry::angRadians;
using Mantid::Geometry::angDegrees;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Functions purely to aid with wrapping
namespace //<unnamed>
{
  using namespace Mantid::PythonInterface;

  /// Pass-through function to return the B matrix as a numpy array
  PyObject * getB(UnitCell& self)
  {
    return Numpy::wrapWithReadOnlyNumpy(self.getB());
  }

  /// Pass-through function to return the B matrix as a numpy array
  PyObject * getG(UnitCell& self)
  {
    return Numpy::wrapWithReadOnlyNumpy(self.getG());
  }


  /// Pass-through function to return the B matrix as a numpy array
  PyObject * getGstar(UnitCell& self)
  {
    return Numpy::wrapWithReadOnlyNumpy(self.getGstar());
  }

  /// Pass-through function to set the unit cell from a 2D numpy array
  void recalculateFromGstar(UnitCell & self, PyObject* values)
  {
    // Create a double matrix and put this in to the unit cell
    self.recalculateFromGstar(Numpy::createDoubleMatrix(values));
  }
}

void export_UnitCell()
{
  enum_<AngleUnits>("AngleUnits")
    .value("Degrees", angDegrees)
    .value("Radians", angRadians)
    .export_values();     

  class_< UnitCell >( "UnitCell", init< >() )    
    .def( init<UnitCell const &>(arg("other") ) )     
    .def( init< double, double, double >(( arg("_a"), arg("_b"), arg("_c") )) )    
    .def( init< double, double, double, double, double, double, optional< int > >(( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(angDegrees) )) )
    .def( "a", (double ( UnitCell::* )() const) &UnitCell::a )
    .def( "a1", (double ( UnitCell::* )() const) &UnitCell::a1 )    
    .def( "a2", (double ( UnitCell::* )() const) &UnitCell::a2 )    
    .def( "a3", (double ( UnitCell::* )() const) &UnitCell::a3 )    
    .def( "alpha", (double ( UnitCell::* )() const) &UnitCell::alpha )    
    .def( "alpha1", (double ( UnitCell::* )() const) &UnitCell::alpha1 )    
    .def( "alpha2", (double ( UnitCell::* )() const) &UnitCell::alpha2 )    
    .def( "alpha3", (double ( UnitCell::* )() const) &UnitCell::alpha3 )    
    .def( "alphastar", (double ( UnitCell::* )() const) &UnitCell::alphastar )    
    .def( "astar", (double ( UnitCell::* )() const) &UnitCell::astar )    
    .def( "b", (double ( UnitCell::* )() const) &UnitCell::b )    
    .def( "b1", (double ( UnitCell::* )() const) &UnitCell::b1 )    
    .def( "b2", (double ( UnitCell::* )() const) &UnitCell::b2 )    
    .def( "b3", (double ( UnitCell::* )() const) &UnitCell::b3 )    
    .def( "beta", (double ( UnitCell::* )() const) &UnitCell::beta )    
    .def( "beta1", (double ( UnitCell::* )() const) &UnitCell::beta1 )    
    .def( "beta2", (double ( UnitCell::* )() const) &UnitCell::beta2 )    
    .def( "beta3", (double ( UnitCell::* )() const) &UnitCell::beta3 )    
    .def( "betastar", (double ( UnitCell::* )() const) &UnitCell::betastar )    
    .def( "bstar", (double ( UnitCell::* )() const) &UnitCell::bstar )    
    .def( "c", (double ( UnitCell::* )() const) &UnitCell::c )    
    .def( "cstar", (double ( UnitCell::* )() const) &UnitCell::cstar )    
    .def( "d", (double ( UnitCell::* )( double,double,double ) const) &UnitCell::d, (arg("h"), arg("k"), arg("l") ))
    .def( "d", (double ( UnitCell::* )(const V3D &) const) &UnitCell::d, (arg("hkl")))
    .def( "dstar", (double ( UnitCell::* )( double,double,double ) const) &UnitCell::dstar , (arg("h"), arg("k"), arg("l") ))    
    .def( "gamma", (double ( UnitCell::* )() const) &UnitCell::gamma )   
    .def( "gammastar", (double ( UnitCell::* )() const) &UnitCell::gammastar )    
    .def( "recAngle", (double ( UnitCell::* )( double,double,double,double,double,double,int const ) const)&UnitCell::recAngle, ( arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"), arg("l2"), arg("Unit")=(int)(angDegrees) ))
    .def( "recVolume", (double ( UnitCell::* )() const) &UnitCell::recVolume )
    .def( "set", (void ( UnitCell::* )( double,double,double,double,double,double,int const)) &UnitCell::set, ( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(angDegrees)))
    .def( "seta", (void ( UnitCell::* )( double ) )( &UnitCell::seta ), ( arg("_a") ) )
    .def( "setalpha", (void ( UnitCell::* )( double,int const ) )( &UnitCell::setalpha ), ( arg("_alpha"), arg("Unit")=(int)(angDegrees) ) )
    .def( "setb", (void ( UnitCell::* )( double ) )( &UnitCell::setb ), ( arg("_b") ) )    
    .def( "setbeta", (void ( UnitCell::* )( double,int const ) )( &UnitCell::setbeta ), ( arg("_beta"), arg("Unit")=(int)(angDegrees) ) )
    .def( "setc", (void ( UnitCell::* )( double ) )( &UnitCell::setc ), ( arg("_c") ) )    
    .def( "setgamma", (void ( UnitCell::* )( double,int const ) )( &UnitCell::setgamma ), ( arg("_gamma"), arg("Unit")=(int)(angDegrees) ) )
    .def( "volume", (double ( UnitCell::* )() const) &UnitCell::volume)
    .def( "getG", &getG)
    .def( "getGstar", &getGstar)
    .def( "getB", &getB )
    .def( "recalculateFromGstar", &recalculateFromGstar)
    ;

    scope().attr("deg2rad") = Mantid::Geometry::deg2rad;
    scope().attr("rad2deg") = Mantid::Geometry::rad2deg;
}

