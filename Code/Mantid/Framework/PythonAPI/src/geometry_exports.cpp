//
// Wrappers for classes in the Mantid::Geometry namespace
//

/** M. Gigg 2010-12-02: gcc 4.4 warns about an aliasing pointer problem when 
  trying to compile the line
 .def("getDistance", &Geometry::IComponent::getDistance)
 It is something to do with the function taking a reference to an IComponent but
 there doesn't seem to be a problem when it is used so disable the warning but
 just for this translation unit
 */ 
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include "MantidPythonAPI/BoostPython_Silent.h"
//Geometry
#include <MantidGeometry/V3D.h>
#include <MantidGeometry/Quat.h>
#include <MantidGeometry/Crystal/UnitCell.h>
#include <MantidGeometry/Instrument/ObjComponent.h>
#include <MantidGeometry/Instrument/Component.h>
#include <MantidGeometry/Instrument/CompAssembly.h>
#include <MantidGeometry/Instrument/Detector.h>
#include <MantidGeometry/Instrument/Instrument.h>
#include <MantidGeometry/Instrument/DetectorGroup.h>
#include "MantidPythonAPI/geometryhelper.h" //exports for matrices to numpy arrays


namespace Mantid
{
  namespace PythonAPI
  {
    using namespace boost::python;


    void export_utils()
    {
      //V3D class
      class_< Geometry::V3D >("V3D",init<>("Construct a V3D at 0,0,0"))
        .def(init<double, double, double>("Construct a V3D with X,Y,Z coordinates"))
        .def("getX", &Geometry::V3D::X, return_value_policy< copy_const_reference >())
        .def("getY", &Geometry::V3D::Y, return_value_policy< copy_const_reference >())
        .def("getZ", &Geometry::V3D::Z, return_value_policy< copy_const_reference >())
        .def("distance", &Geometry::V3D::distance)
        .def("angle", &Geometry::V3D::angle)
        .def("zenith", &Geometry::V3D::zenith)
        .def("scalar_prod", &Geometry::V3D::scalar_prod)
        .def("cross_prod", &Geometry::V3D::cross_prod)
        .def("norm", &Geometry::V3D::norm)
        .def("norm2", &Geometry::V3D::norm2)
        .def(self + self)
        .def(self += self)
        .def(self - self)
        .def(self -= self)
        .def(self * self)
        .def(self *= self)
        .def(self / self)
        .def(self /= self)
        .def(self * int())
        .def(self *= int())
        .def(self * double())
        .def(self *= double())
        .def(self < self)
        .def(self == self)
        .def(self_ns::str(self))
        ;

      //Quat class
      class_< Geometry::Quat >("Quat", init<>("Construct a default Quat that will perform no transformation."))
        .def(init<double, double, double, double>("Constructor with values"))
        .def(init<V3D, V3D>("Construct a Quat between two vectors"))
        .def(init<V3D, V3D, V3D>("Construct a Quaternion that performs a reference frame rotation.\nThe initial X,Y,Z vectors are aligned as expected: X=(1,0,0), Y=(0,1,0), Z=(0,0,1)"))
        .def(init<double,V3D>("Constructor from an angle(degrees) and an axis."))
        .def("rotate", &Geometry::Quat::rotate)
        .def("real", &Geometry::Quat::real)
        .def("imagI", &Geometry::Quat::imagI)
        .def("imagJ", &Geometry::Quat::imagJ)
        .def("imagK", &Geometry::Quat::imagK)
        .def(self + self)
        .def(self += self)
        .def(self - self)
        .def(self -= self)
        .def(self * self)
        .def(self *= self)
        .def(self == self)
        .def(self != self)
        .def(self_ns::str(self))
        ;
    }


    // Default parameter function overloads
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getParameterNames,Geometry::Component::getParameterNames,0,1);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_hasParameter,Geometry::Component::hasParameter,1,2);

    //
    // MG: Ticket #1601 is going to clean up the parameter access mess but the recursion takes precendence.
    //
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getNumberParameter,Geometry::Component::getNumberParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getPositionParameter,Geometry::Component::getPositionParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRotationParameter,Geometry::Component::getRotationParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getStringParameter,Geometry::Component::getStringParameter,1,2);
    
    void export_components()
    {
      /**
      * Both the interface and concrete implementations need to be exposed here so that Python
      * sees the correct object type and not just always IComponent. This enables getComponentByName
      * to return an IComponent pointer in C++ while still having an underlying object in Python that will accept
      * the usage of ICompAssembly methods
      */
      //IComponent class
      register_ptr_to_python<boost::shared_ptr<Geometry::IComponent> >();

      //
      // MG: Ticket #1601 is going to clean up the parameter access mess but the recursion takes precendence.
      //

      class_<Geometry::IComponent, boost::noncopyable>("IComponent", no_init)
        .def("getPos", &Geometry::IComponent::getPos)
        .def("getDistance", &Geometry::IComponent::getDistance)
        .def("getName", &Geometry::IComponent::getName)
        .def("type", &Geometry::IComponent::type)
        ;

      //ICompAssembly class
      register_ptr_to_python<boost::shared_ptr<Geometry::ICompAssembly> >();

      class_<Geometry::ICompAssembly, boost::python::bases<Geometry::IComponent>, 
        boost::noncopyable>("ICompAssembly", no_init)
        .def("nElements", &Geometry::ICompAssembly::nelements)
        .def("__getitem__", &Geometry::ICompAssembly::operator[])
        ;

      //IObjComponent class
      register_ptr_to_python<boost::shared_ptr<Geometry::IObjComponent> >();

      class_< Geometry::IObjComponent, boost::python::bases<Geometry::IComponent>, 
        boost::noncopyable>("IObjComponent", no_init)
        ;

      //IDetector Class
      register_ptr_to_python<boost::shared_ptr<Geometry::IDetector> >();

      class_< Geometry::IDetector, bases<Geometry::IObjComponent>,
        boost::noncopyable>("IDetector", no_init)
        .def("getID", &Geometry::IDetector::getID)
        .def("isMasked", &Geometry::IDetector::isMasked)
        .def("isMonitor", &Geometry::IDetector::isMonitor)
        .def("solidAngle", &Geometry::IDetector::solidAngle)
        .def("getTwoTheta", &Geometry::IDetector::getTwoTheta)
        .def("getPhi", &Geometry::IDetector::getPhi)
        ;

      /**
      * Concrete implementations
      */
      /**
      * MG 14/07/2010 - These classes do not offer any additional functionality they just allow 
      * Python to cast an object of a base-class to one of the derived types
      */

      //Component class
      class_<Geometry::Component, bases<Geometry::IComponent>, boost::noncopyable>("Component", no_init)
        .def("getParameterNames", &Geometry::Component::getParameterNames, Component_getParameterNames())
        .def("hasParameter", &Geometry::Component::hasParameter, Component_hasParameter())
        .def("getNumberParameter", &Geometry::Component::getNumberParameter, Component_getNumberParameter())
        .def("getPositionParameter", &Geometry::Component::getPositionParameter, Component_getPositionParameter())
        .def("getRotationParameter", &Geometry::Component::getRotationParameter, Component_getRotationParameter())
        .def("getStringParameter", &Geometry::Component::getStringParameter, Component_getStringParameter())

        ;


      /** 
      * The detector classes are prefixed with Mantid due to an unfortunate name clash in an existing user script
      * where a function was already called Detector. Given that the name here should rarely be used
      * this seemed like an acceptable compromise.
      */
      //Detector
      class_<Geometry::Detector, bases<Geometry::IDetector,Geometry::Component>,
        boost::noncopyable>("MantidDetector", no_init)
        ;
      
      //CompAssembly class
      class_<Geometry::CompAssembly, boost::python::bases<Geometry::Component, Geometry::ICompAssembly>, 
        boost::noncopyable>("CompAssembly", no_init)
        ;

      //DetectorGroup
      class_<Geometry::DetectorGroup, bases<Geometry::IDetector>, boost::noncopyable>("DetectorGroup", no_init)
        .def("getDetectorIDs", &Geometry::DetectorGroup::getDetectorIDs)
        ;
    }

  void export_instrument()
  {
    //Pointer to the interface
    register_ptr_to_python<boost::shared_ptr<Geometry::IInstrument> >();
    
    //IInstrument class
    class_< Geometry::IInstrument, boost::python::bases<Geometry::ICompAssembly>, 
      boost::noncopyable>("IInstrument", no_init)
      .def("getSample", &Geometry::IInstrument::getSample)
      .def("getSource", &Geometry::IInstrument::getSource)
      .def("getComponentByName", &Geometry::IInstrument::getComponentByName)
      .def("getDetector", (IDetector_sptr (IInstrument::*)(const detid_t&)const)&Geometry::IInstrument::getDetector)
      ;

    /** Concrete implementations so that Python knows about them */
    
    //Instrument class
    class_< Geometry::Instrument, boost::python::bases<Geometry::IInstrument, Geometry::CompAssembly>, 
	    boost::noncopyable>("Instrument", no_init)
      ;
    
  }

  void export_unit_cell()
  {
    /// UnitCell class Python bindings. If a function returns a matrix, the UnitCellWrapper class is used instead
    enum_< Geometry::AngleUnits>("AngleUnits")
       .value("Degrees", Geometry::angDegrees)
       .value("Radians", Geometry::angRadians)
       .export_values();     

    class_< Geometry::UnitCell >( "UnitCell", init< >() )    
      .def( init< Geometry::UnitCell const & >(( arg("other") )) )    
      .def( init< double, double, double >(( arg("_a"), arg("_b"), arg("_c") )) )    
      .def( init< double, double, double, double, double, double, optional< int > >(( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) )) )
      .def( "a", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::a ) )    
      .def( "a1", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::a1 ) )    
      .def( "a2", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::a2 ) )    
      .def( "a3", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::a3 ) )    
      .def( "alpha", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::alpha ) )    
      .def( "alpha1", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::alpha1 ) )    
      .def( "alpha2", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::alpha2 ) )    
      .def( "alpha3", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::alpha3 ) )    
      .def( "alphastar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::alphastar ) )    
      .def( "astar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::astar ) )    
      .def( "b", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::b ) )    
      .def( "b1", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::b1 ) )    
      .def( "b2", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::b2 ) )    
      .def( "b3", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::b3 ) )    
      .def( "beta", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::beta ) )    
      .def( "beta1", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::beta1 ) )    
      .def( "beta2", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::beta2 ) )    
      .def( "beta3", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::beta3 ) )    
      .def( "betastar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::betastar ) )    
      .def( "bstar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::bstar ) )    
      .def( "c", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::c ) )    
      .def( "cstar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::cstar ) )    
      .def( "d", (double ( Geometry::UnitCell::* )( double,double,double ) const)( &Geometry::UnitCell::d ), (arg("h"), arg("k"), arg("l") ) )    
      .def( "dstar", (double ( Geometry::UnitCell::* )( double,double,double ) const)( &Geometry::UnitCell::dstar ), (arg("h"), arg("k"), arg("l") ) )    
      .def( "gamma", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::gamma ) )    
      .def( "gammastar", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::gammastar ) )    
      .def( "recAngle", (double ( Geometry::UnitCell::* )( double,double,double,double,double,double,int const ) const)( &Geometry::UnitCell::recAngle ), ( arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"), arg("l2"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "recVolume", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::recVolume ) )    
      .def( "set", (void ( Geometry::UnitCell::* )( double,double,double,double,double,double,int const ) )( &Geometry::UnitCell::set ), ( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "seta", (void ( Geometry::UnitCell::* )( double ) )( &Geometry::UnitCell::seta ), ( arg("_a") ) )    
      .def( "setalpha", (void ( Geometry::UnitCell::* )( double,int const ) )( &Geometry::UnitCell::setalpha ), ( arg("_alpha"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "setb", (void ( Geometry::UnitCell::* )( double ) )( &Geometry::UnitCell::setb ), ( arg("_b") ) )    
      .def( "setbeta", (void ( Geometry::UnitCell::* )( double,int const ) )( &Geometry::UnitCell::setbeta ), ( arg("_beta"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "setc", (void ( Geometry::UnitCell::* )( double ) )( &Geometry::UnitCell::setc ), ( arg("_c") ) )    
      .def( "setgamma", (void ( Geometry::UnitCell::* )( double,int const ) )( &Geometry::UnitCell::setgamma ), ( arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "volume", (double ( Geometry::UnitCell::* )(  ) const)( &Geometry::UnitCell::volume ) )
      .def( "getG",( &UnitCellWrapper::getG ) )
      .def( "getGstar",( &UnitCellWrapper::getGstar ) )
      .def( "getB",( &UnitCellWrapper::getB ) );

      scope().attr("deg2rad") = Geometry::deg2rad;
      scope().attr("rad2deg") = Geometry::rad2deg;
    }

    void export_geometry_namespace()
    {
      export_utils();
      export_components();
      export_instrument();
      export_unit_cell();
    }

  }
}
