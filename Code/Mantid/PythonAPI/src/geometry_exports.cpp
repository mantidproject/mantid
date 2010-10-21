//
// Wrappers for classes in the Mantid::Geometry namespace
//

#include <boost/python.hpp>
//Geometry
#include <MantidGeometry/V3D.h>
#include <MantidGeometry/Quat.h>

#include <MantidGeometry/Instrument/ObjComponent.h>
#include <MantidGeometry/Instrument/ParObjComponent.h>
#include <MantidGeometry/Instrument/Component.h>
#include <MantidGeometry/Instrument/ParametrizedComponent.h>
#include <MantidGeometry/Instrument/CompAssembly.h>
#include <MantidGeometry/Instrument/ParCompAssembly.h>
#include <MantidGeometry/Instrument/Detector.h>
#include <MantidGeometry/Instrument/ParDetector.h>
#include <MantidGeometry/Instrument/Instrument.h>
#include <MantidGeometry/Instrument/ParInstrument.h>


namespace Mantid
{
  namespace PythonAPI
  {
    using namespace boost::python;


    void export_utils()
    {
      //V3D class
      class_< Geometry::V3D >("V3D", init<double, double, double>())
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
      class_< Geometry::Quat >("Quaternion", init<double, double, double, double>())
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
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_getParameterNames,Geometry::ParametrizedComponent::getParameterNames,0,1);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_hasParameter,Geometry::Component::hasParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_hasParameter,Geometry::ParametrizedComponent::hasParameter,1,2);

    //
    // MG: Ticket #1601 is going to clean up the parameter access mess but the recursion takes precendence.
    //
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getNumberParameter,Geometry::Component::getNumberParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_getNumberParameter,Geometry::ParametrizedComponent::getNumberParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getPositionParameter,Geometry::Component::getPositionParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_getPositionParameter,Geometry::ParametrizedComponent::getPositionParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getRotationParameter,Geometry::Component::getRotationParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_getRotationParameter,Geometry::ParametrizedComponent::getRotationParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Component_getStringParameter,Geometry::Component::getStringParameter,1,2);
    BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ParComponent_getStringParameter,Geometry::ParametrizedComponent::getStringParameter,1,2);
    
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
      //ParameterizedComponent
      class_<Geometry::ParametrizedComponent, bases<Geometry::IComponent>, 
        boost::noncopyable>("ParameterizedComponent", no_init)
        .def("getParameterNames", &Geometry::ParametrizedComponent::getParameterNames, ParComponent_getParameterNames())
        .def("hasParameter", &Geometry::ParametrizedComponent::hasParameter, ParComponent_hasParameter())
        .def("getNumberParameter", &Geometry::ParametrizedComponent::getNumberParameter, ParComponent_getNumberParameter())
        .def("getPositionParameter", &Geometry::ParametrizedComponent::getPositionParameter, ParComponent_getPositionParameter())
        .def("getRotationParameter", &Geometry::ParametrizedComponent::getRotationParameter, ParComponent_getRotationParameter())
        .def("getStringParameter", &Geometry::ParametrizedComponent::getStringParameter, ParComponent_getStringParameter())

        ;

      /** 
      * The detector classes are prefixed with Mantid due to an unfortunate name clash in an existing user script
      * where a function was already called Detector. Given that the name here should rarely be used
      * this seemed like an acceptable compromise.
      */
      //Detector
      class_<Geometry::Detector, bases<Geometry::IDetector>,
        boost::noncopyable>("MantidDetector", no_init)
        ;
      //Detector
      class_<Geometry::ParDetector, bases<Geometry::IDetector>,
        boost::noncopyable>("MantidParDetector", no_init)
        ;

      //CompAssembly class
      class_<Geometry::CompAssembly, boost::python::bases<Geometry::Component, Geometry::ICompAssembly>, 
        boost::noncopyable>("CompAssembly", no_init)
        ;
      //ParCompAssembly class
      class_<Geometry::ParCompAssembly, boost::python::bases<Geometry::ParametrizedComponent,Geometry::ICompAssembly>, 
        boost::noncopyable>("ParCompAssembly", no_init)
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
      ;

    /** Concrete implementations so that Python knows about them */
    
    //Instrument class
    class_< Geometry::Instrument, boost::python::bases<Geometry::IInstrument, Geometry::CompAssembly>, 
	    boost::noncopyable>("Instrument", no_init)
      ;
    //Instrument class
    class_< Geometry::ParInstrument, boost::python::bases<Geometry::IInstrument, Geometry::ParCompAssembly>, 
	    boost::noncopyable>("ParInstrument", no_init)
      ;
  }


    void export_geometry_namespace()
    {
      export_utils();
      export_components();
      export_instrument();
    }

    //@endcond
  }
}
