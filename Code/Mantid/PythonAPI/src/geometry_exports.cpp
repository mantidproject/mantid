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

namespace Mantid
{
namespace PythonAPI
{
  using namespace boost::python;
  
  //@cond
  void export_utils()
  {
    //V3D class
    class_< Mantid::Geometry::V3D >("V3D", init<double, double, double>())
      .def("getX", &Mantid::Geometry::V3D::X, return_value_policy< copy_const_reference >())
      .def("getY", &Mantid::Geometry::V3D::Y, return_value_policy< copy_const_reference >())
      .def("getZ", &Mantid::Geometry::V3D::Z, return_value_policy< copy_const_reference >())
      .def("distance", &Mantid::Geometry::V3D::distance)
      .def("angle", &Mantid::Geometry::V3D::angle)
      .def("zenith", &Mantid::Geometry::V3D::zenith)
      .def("scalar_prod", &Mantid::Geometry::V3D::scalar_prod)
      .def("cross_prod", &Mantid::Geometry::V3D::cross_prod)
      .def("norm", &Mantid::Geometry::V3D::norm)
      .def("norm2", &Mantid::Geometry::V3D::norm2)
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
    class_< Mantid::Geometry::Quat >("Quaternion", init<double, double, double, double>())
      .def("rotate", &Mantid::Geometry::Quat::rotate)
      .def("real", &Mantid::Geometry::Quat::real)
      .def("imagI", &Mantid::Geometry::Quat::imagI)
      .def("imagJ", &Mantid::Geometry::Quat::imagJ)
      .def("imagK", &Mantid::Geometry::Quat::imagK)
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

  void export_components()
  {
    /**
     * Both the interface and concrete implementations need to be exposed here so that Python
     * sees the correct object type and not just always IComponent. This enables getComponentByName
     * to return an IComponent pointer in C++ while still having an underlying object in Python that will accept
     * the usage of ICompAssembly methods
     */
    //IComponent class
    register_ptr_to_python<boost::shared_ptr<Mantid::Geometry::IComponent> >();
    
    class_<Mantid::Geometry::IComponent, boost::noncopyable>("IComponent", no_init)
      .def("getPos", &Mantid::Geometry::IComponent::getPos)
      .def("getName", &Mantid::Geometry::IComponent::getName)
      .def("type", &Mantid::Geometry::IComponent::type)
      .def("getNumberParameter", &Mantid::Geometry::IComponent::getNumberParameter)
      .def("getPositionParameter", &Mantid::Geometry::IComponent::getPositionParameter)
      .def("getRotationParameter", &Mantid::Geometry::IComponent::getRotationParameter)
	  .def("getStringParameter", &Mantid::Geometry::IComponent::getStringParameter)
      ;
   
    //ICompAssembly class
    register_ptr_to_python<boost::shared_ptr<Mantid::Geometry::ICompAssembly> >();
    
    class_<Mantid::Geometry::ICompAssembly, boost::python::bases<Mantid::Geometry::IComponent>, 
      boost::noncopyable>("ICompAssembly", no_init)
      .def("nElements", &Mantid::Geometry::ICompAssembly::nelements)
      .def("__getitem__", &Mantid::Geometry::ICompAssembly::operator[])
      ;
    
    //IObjComponent class
    register_ptr_to_python<boost::shared_ptr<Mantid::Geometry::IObjComponent> >();
    
    class_< Mantid::Geometry::IObjComponent, boost::python::bases<Mantid::Geometry::IComponent>, 
      boost::noncopyable>("IObjComponent", no_init)
    ;
  
    //IDetector Class
    register_ptr_to_python<boost::shared_ptr<Mantid::Geometry::IDetector> >();
  
    class_< Mantid::Geometry::IDetector, bases<Mantid::Geometry::IObjComponent>,
      boost::noncopyable>("IDetector", no_init)
      .def("getID", &Mantid::Geometry::IDetector::getID)
      .def("isMasked", &Mantid::Geometry::IDetector::isMasked)
      .def("isMonitor", &Mantid::Geometry::IDetector::isMonitor)
      .def("solidAngle", &Mantid::Geometry::IDetector::solidAngle)
      .def("getTwoTheta", &Mantid::Geometry::IDetector::getTwoTheta)
      .def("getPhi", &Mantid::Geometry::IDetector::getPhi)
      ;

    /**
     * Concrete implementations
     */
    //Component class
    class_<Mantid::Geometry::Component, bases<Mantid::Geometry::IComponent>, boost::noncopyable>("Component", no_init)
      ;
    //ParameterizedComponent
    class_<Mantid::Geometry::ParametrizedComponent, bases<Mantid::Geometry::IComponent>, 
      boost::noncopyable>("ParameterizedComponent", no_init)
      ;

    /**
     * MG 14/07/2010 - These classes do not offer any additional functionality they just allow 
     * Python to recogise that an object of this type may exist. The detector class 
     * is prefixed with Mantid due to an unfortunate name class in an existing user script (SANS)
     * where a function was already called Detector. Given that the name here should rarely be used
     * this seemed like an acceptable compromise.
     */
	//Detector
	class_<Mantid::Geometry::Detector, bases<Mantid::Geometry::IDetector>,
		boost::noncopyable>("MantidDetector", no_init)
		;
	//Detector
	class_<Mantid::Geometry::ParDetector, bases<Mantid::Geometry::IDetector>,
		boost::noncopyable>("MantidParDetector", no_init)
		;

    //CompAssembly class
    class_<Mantid::Geometry::CompAssembly, boost::python::bases<Mantid::Geometry::ICompAssembly>, 
      boost::noncopyable>("CompAssembly", no_init)
      ;
    //ParCompAssembly class
    class_<Mantid::Geometry::ParCompAssembly, boost::python::bases<Mantid::Geometry::ICompAssembly>, 
	   boost::noncopyable>("ParCompAssembly", no_init)
      ;
  }

  void export_geometry_namespace()
  {
    export_utils();
    export_components();
  }

  //@endcond
}
}
