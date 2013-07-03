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
#include <MantidGeometry/Crystal/UnitCell.h>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include <MantidGeometry/Instrument/Goniometer.h>
#include <MantidGeometry/Instrument/ObjComponent.h>
#include <MantidGeometry/Instrument/ObjCompAssembly.h>
#include <MantidGeometry/Instrument/Component.h>
#include <MantidGeometry/Instrument/CompAssembly.h>
#include <MantidGeometry/Instrument/Detector.h>
#include <MantidGeometry/Instrument.h>
#include <MantidGeometry/Instrument/DetectorGroup.h>
#include <MantidGeometry/Instrument/ReferenceFrame.h>
#include "MantidPythonAPI/geometryhelper.h" //exports for matrices to numpy arrays

namespace Mantid
{
  namespace PythonAPI
  {
    using namespace boost::python;

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
        .def("getFullName", &Geometry::IComponent::getFullName) 
        .def("type", &Geometry::IComponent::type)
        ;

      //ICompAssembly class
      register_ptr_to_python<boost::shared_ptr<Geometry::ICompAssembly> >();

      class_<Geometry::ICompAssembly, boost::python::bases<Geometry::IComponent>, 
        boost::noncopyable>("ICompAssembly", no_init)
        .def("nElements", &Geometry::ICompAssembly::nelements)
        .def("__getitem__", &Geometry::ICompAssembly::operator[])
        ;

      //ObjCompAssembly class
      register_ptr_to_python<boost::shared_ptr<Geometry::ObjCompAssembly> >();

      class_<Geometry::ObjCompAssembly, boost::python::bases<Geometry::IComponent>,
        boost::noncopyable>("IObjCompAssembly", no_init)
        .def("nElements", &Geometry::ObjCompAssembly::nelements)
        .def("__getitem__", &Geometry::ObjCompAssembly::operator[])
        ;
      //IObjComponent class
      register_ptr_to_python<boost::shared_ptr<Geometry::IObjComponent> >();

      class_< Geometry::IObjComponent, boost::python::bases<Geometry::IComponent>, 
        boost::noncopyable>("IObjComponent", no_init)
        ;

      //IDetector Class
      register_ptr_to_python<boost::shared_ptr<Geometry::IDetector> >();
      register_ptr_to_python<boost::shared_ptr<const Geometry::IDetector> >();
      implicitly_convertible<boost::shared_ptr<Geometry::IDetector>, boost::shared_ptr<const Geometry::IDetector> >();

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

  void export_reference_frame()
  {
    register_ptr_to_python<boost::shared_ptr<const Geometry::ReferenceFrame> >();

    enum_< Geometry::PointingAlong>("PointingAlong")
       .value("X", Geometry::X)
       .value("Y", Geometry::Y)
       .value("Z", Geometry::Z)
       .export_values();  

    class_< Geometry::ReferenceFrame, boost::noncopyable>("ReferenceFrame", no_init)
      .def( "pointingAlongBeam", &Geometry::ReferenceFrame::pointingAlongBeam)
      .def( "pointingUp", &Geometry::ReferenceFrame::pointingUp)
      .def( "vecPointingUp", &Geometry::ReferenceFrame::vecPointingUp )
      .def( "vecPointingAlongBeam", &Geometry::ReferenceFrame::vecPointingAlongBeam )
      ;
  }

  void export_instrument()
  {
    //Pointer to the interface
    register_ptr_to_python<boost::shared_ptr<Geometry::Instrument> >();
    
    //Instrument class
    class_< Geometry::Instrument, boost::python::bases<Geometry::CompAssembly>,
      boost::noncopyable>("Instrument", no_init)
      .def("getSample", (boost::shared_ptr<Geometry::IObjComponent> (Geometry::Instrument::*)())&Geometry::Instrument::getSample)
      .def("getSource", (boost::shared_ptr<Geometry::IObjComponent> (Geometry::Instrument::*)())&Geometry::Instrument::getSource)
      .def("getComponentByName", (boost::shared_ptr<Geometry::IComponent> (Geometry::Instrument::*)(const std::string&))&Geometry::Instrument::getComponentByName)
      .def("getDetector", (boost::shared_ptr<Geometry::IDetector> (Geometry::Instrument::*)(const detid_t&)const)&Geometry::Instrument::getDetector)
      .def("getReferenceFrame", (boost::shared_ptr<const Geometry::ReferenceFrame> (Geometry::Instrument::*)())&Geometry::Instrument::getReferenceFrame )
      .def("getValidFromDate", &Geometry::Instrument::getValidFromDate, "Return the valid from date of the instrument")
      .def("getValidToDate", &Geometry::Instrument::getValidToDate, "Return the valid to date of the instrument")
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
      .def( "getB",( &UnitCellWrapper::getB ) )
      .def( "recalculateFromGstar", ( &UnitCellWrapper::recalculateFromGStar ) ) ;

      scope().attr("deg2rad") = Geometry::deg2rad;
      scope().attr("rad2deg") = Geometry::rad2deg;
    }

  void export_oriented_lattice()
  {
    /// OrientedLattice class Python bindings. If a function uses or returns a matrix, the OrientedLatticeWrapper class is used instead   
    //all functions inherited from UnitCell first
    class_< Geometry::OrientedLattice >( "OrientedLattice", init< >() )    
      .def( init< Geometry::OrientedLattice const & >(( arg("other") )) )    
      .def( init< double, double, double >(( arg("_a"), arg("_b"), arg("_c") )) )    
      .def( init< double, double, double, double, double, double, optional< int > >(( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) )) )
      .def( "a", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::a ) )    
      .def( "a1", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::a1 ) )    
      .def( "a2", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::a2 ) )    
      .def( "a3", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::a3 ) )    
      .def( "alpha", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::alpha ) )    
      .def( "alpha1", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::alpha1 ) )    
      .def( "alpha2", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::alpha2 ) )    
      .def( "alpha3", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::alpha3 ) )    
      .def( "alphastar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::alphastar ) )    
      .def( "astar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::astar ) )    
      .def( "b", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::b ) )    
      .def( "b1", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::b1 ) )    
      .def( "b2", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::b2 ) )    
      .def( "b3", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::b3 ) )    
      .def( "beta", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::beta ) )    
      .def( "beta1", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::beta1 ) )    
      .def( "beta2", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::beta2 ) )    
      .def( "beta3", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::beta3 ) )    
      .def( "betastar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::betastar ) )    
      .def( "bstar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::bstar ) )    
      .def( "c", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::c ) )    
      .def( "cstar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::cstar ) )    
      .def( "d", (double ( Geometry::OrientedLattice::* )( double,double,double ) const)( &Geometry::OrientedLattice::d ), (arg("h"), arg("k"), arg("l") ) )    
      .def( "dstar", (double ( Geometry::OrientedLattice::* )( double,double,double ) const)( &Geometry::OrientedLattice::dstar ), (arg("h"), arg("k"), arg("l") ) )    
      .def( "gamma", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::gamma ) )    
      .def( "gammastar", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::gammastar ) )    
      .def( "recAngle", (double ( Geometry::OrientedLattice::* )( double,double,double,double,double,double,int const ) const)( &Geometry::OrientedLattice::recAngle ), ( arg("h1"), arg("k1"), arg("l1"), arg("h2"), arg("k2"), arg("l2"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "recVolume", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::recVolume ) )    
      .def( "set", (void ( Geometry::OrientedLattice::* )( double,double,double,double,double,double,int const ) )( &Geometry::OrientedLattice::set ), ( arg("_a"), arg("_b"), arg("_c"), arg("_alpha"), arg("_beta"), arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "seta", (void ( Geometry::OrientedLattice::* )( double ) )( &Geometry::OrientedLattice::seta ), ( arg("_a") ) ) 
      .def( "setalpha", (void ( Geometry::OrientedLattice::* )( double,int const ) )( &Geometry::OrientedLattice::setalpha ), ( arg("_alpha"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "setb", (void ( Geometry::OrientedLattice::* )( double ) )( &Geometry::OrientedLattice::setb ), ( arg("_b") ) )    
      .def( "setbeta", (void ( Geometry::OrientedLattice::* )( double,int const ) )( &Geometry::OrientedLattice::setbeta ), ( arg("_beta"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "setc", (void ( Geometry::OrientedLattice::* )( double ) )( &Geometry::OrientedLattice::setc ), ( arg("_c") ) )    
      .def( "setgamma", (void ( Geometry::OrientedLattice::* )( double,int const ) )( &Geometry::OrientedLattice::setgamma ), ( arg("_gamma"), arg("Unit")=(int)(Geometry::angDegrees) ) )
      .def( "volume", (double ( Geometry::OrientedLattice::* )(  ) const)( &Geometry::OrientedLattice::volume ) )
      .def( "getG",( &OrientedLatticeWrapper::getG ) )
      .def( "getGstar",( &OrientedLatticeWrapper::getGstar ) )
      .def( "getB",( &OrientedLatticeWrapper::getB ) )
//      .def( "recalculateFromGstar", ( &OrientedLatticeWrapper::recalculateFromGstar ) )
// new functions speific to oriented lattice
      .def( init< Geometry::UnitCell >(( arg("uc") )) )  
      .def( "getU",( &OrientedLatticeWrapper::getU ) )
      .def( "setU", ( &OrientedLatticeWrapper::setU ) ) 
      .def( "getUB",( &OrientedLatticeWrapper::getUB ) )
      .def( "setUB", ( &OrientedLatticeWrapper::setUB ) )
      .def( "getuVector", (&Geometry::OrientedLattice::getuVector))
      .def( "getvVector", (&Geometry::OrientedLattice::getvVector))
      .def( "setUFromVectors", ( &OrientedLatticeWrapper::setUFromVectors ) )
        ;
    }

    namespace
    {
      PyObject *getR(Geometry::Goniometer &self)
      {
        return MantidVecHelper::createPythonWrapper(self.getR(), true);
      }
    }

    void export_goniometer()
    {
      enum_< Geometry::RotationSense>("RotationSense")
       .value("CW", Geometry::CW)
       .value("CCW", Geometry::CCW)
       .export_values(); 


      class_< Geometry::GoniometerAxis>("GoniometerAxis", no_init)
        .add_property("name", &Geometry::GoniometerAxis::name)
        .add_property("rotationaxis", &Geometry::GoniometerAxis::rotationaxis)
        .add_property("angle", &Geometry::GoniometerAxis::angle)
        .add_property("sense", &Geometry::GoniometerAxis::sense)
        .add_property("angleunit", &Geometry::GoniometerAxis::angleunit)
        ;

      class_< Geometry::Goniometer>( "Goniometer", no_init )  
      .def( init< Geometry::Goniometer const & >(( arg("other") )) )    
      .def( "getR", &getR )
      .def( "axesInfo", &Geometry::Goniometer::axesInfo )  
      .def( "pushAxis", (void ( Geometry::Goniometer::* ) ( std::string, double, double, double, double, int, int)  )(&Geometry::Goniometer::pushAxis), ( arg("name"), arg("axisX"), arg("axisY"), arg("axisZ"), arg("angle"), arg("sense")=(int)(Geometry::CCW), arg("angleUnit")=(int)(Geometry::angDegrees) ) )
      .def( "setRotationAngle", (void ( Geometry::Goniometer::* )( std::string, double) ) (&Geometry::Goniometer::setRotationAngle), (arg("name"), arg("angle") ) )
      .def( "setRotationAngle", (void ( Geometry::Goniometer::* )( size_t, double) ) (&Geometry::Goniometer::setRotationAngle), (arg("axisNumber"), arg("angle") ) )
      .def( "getAxis", (const Geometry::GoniometerAxis &(Geometry::Goniometer::*)(std::string) const) (&Geometry::Goniometer::getAxis), (arg("name") ),
            return_value_policy<copy_const_reference>())
      .def( "getNumberAxes", &Geometry::Goniometer::getNumberAxes, (arg("axisname") ) )
      .def( "makeUniversalGoniometer", (void ( Geometry::Goniometer::*)() ) (&Geometry::Goniometer::makeUniversalGoniometer) )
      .def( "getEulerAngles", (std::vector<double> ( Geometry::Goniometer::* )(std::string) ) (&Geometry::Goniometer::getEulerAngles), (arg("convention")=(std::string)("XYZ") ) )
      ;
    }

    void export_geometry_namespace()
    {
      export_reference_frame();
      export_components();
      export_instrument();
      export_unit_cell();
      export_oriented_lattice();
      export_goniometer();
    }

  }
}
