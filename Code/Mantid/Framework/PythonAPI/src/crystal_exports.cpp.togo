//
// Wrappers for classes in the Mantid::Crystal namespace
//

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <boost/python.hpp>
//Crystal
#include <MantidCrystal/UnitCell.h>
#include <MantidGeometry/Math/Matrix.h>

namespace Mantid
{
  namespace PythonAPI
  {
    using namespace boost::python;


    void export_unit_cell()
    {
      //UnitCell class
      enum_< Mantid::Crystal::AngleUnits>("AngleUnits")
        .value("Degrees", Mantid::Crystal::Degrees)
        .value("Radians", Mantid::Crystal::Radians)
        .export_values()
        ;     

    class_< Mantid::Crystal::UnitCell >( "UnitCell", init< >() )    
          .def( init< Mantid::Crystal::UnitCell const & >(( arg("other") )) )    
        .def( init< double, double, double >(( arg("_a"), arg("_b"), arg("_c") )) )    
        .def( init< double, double, double, double, double, double, optional< int > >(( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) )) )    
        .def( 
            "a"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::a ) )    
        .def( 
            "a1"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::a1 ) )    
        .def( 
            "a2"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::a2 ) )    
        .def( 
            "a3"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::a3 ) )    
        .def( 
            "alpha"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::alpha ) )    
        .def( 
            "alpha1"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::alpha1 ) )    
        .def( 
            "alpha2"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::alpha2 ) )    
        .def( 
            "alpha3"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::alpha3 ) )    
        .def( 
            "alphastar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::alphastar ) )    
        .def( 
            "astar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::astar ) )    
        .def( 
            "b"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::b ) )    
        .def( 
            "b1"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::b1 ) )    
        .def( 
            "b2"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::b2 ) )    
        .def( 
            "b3"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::b3 ) )    
        .def( 
            "beta"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::beta ) )    
        .def( 
            "beta1"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::beta1 ) )    
        .def( 
            "beta2"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::beta2 ) )    
        .def( 
            "beta3"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::beta3 ) )    
        .def( 
            "betastar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::betastar ) )    
        .def( 
            "bstar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::bstar ) )    
        .def( 
            "c"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::c ) )    
        .def( 
            "cstar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::cstar ) )    
        .def( 
            "d"
            , (double ( ::Mantid::Crystal::UnitCell::* )( double,double,double ) const)( &::Mantid::Crystal::UnitCell::d )
            , (arg("h"), arg("k"), arg("l") ) )    
        .def( 
            "dstar"
            , (double ( ::Mantid::Crystal::UnitCell::* )( double,double,double ) const)( &::Mantid::Crystal::UnitCell::dstar )
            , (arg("h"), arg("k"), arg("l") ) )    
        .def( 
            "gamma"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::gamma ) )    
        .def( 
            "gammastar"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::gammastar ) )    
        .def( 
            "recAngle"
            , (double ( ::Mantid::Crystal::UnitCell::* )( double,double,double,double,double,double,int const ) const)( &::Mantid::Crystal::UnitCell::recAngle )
            , ( arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"), arg("l2"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) ) )    
        .def( 
            "recVolume"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::recVolume ) )    
        .def( 
            "set"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double,double,double,double,double,double,int const ) )( &::Mantid::Crystal::UnitCell::set )
            , ( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) ) )    
        .def( 
            "seta"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double ) )( &::Mantid::Crystal::UnitCell::seta )
            , ( arg("_a") ) )    
        .def( 
            "setalpha"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double,int const ) )( &::Mantid::Crystal::UnitCell::setalpha )
            , ( arg("_alpha"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) ) )    
        .def( 
            "setb"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double ) )( &::Mantid::Crystal::UnitCell::setb )
            , ( arg("_b") ) )    
        .def( 
            "setbeta"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double,int const ) )( &::Mantid::Crystal::UnitCell::setbeta )
            , ( arg("_beta"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) ) )    
        .def( 
            "setc"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double ) )( &::Mantid::Crystal::UnitCell::setc )
            , ( arg("_c") ) )    
        .def( 
            "setgamma"
            , (void ( ::Mantid::Crystal::UnitCell::* )( double,int const ) )( &::Mantid::Crystal::UnitCell::setgamma )
            , ( arg("_gamma"), arg("Unit")=(int const)((int const)(::Mantid::Crystal::Degrees)) ) )    
        .def( 
            "volume"
            , (double ( ::Mantid::Crystal::UnitCell::* )(  ) const)( &::Mantid::Crystal::UnitCell::volume ) )
        ;

      scope().attr("deg2rad") = Mantid::Crystal::deg2rad;
      scope().attr("rad2deg") = Mantid::Crystal::rad2deg;
    }


    void export_crystal_namespace()
    {
      export_unit_cell();
    }

  }
}
