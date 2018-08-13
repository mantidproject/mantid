#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::ComponentInfo;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using namespace boost::python;
namespace Converters = Mantid::PythonInterface::Converters;

// Helper function to call a converter for std::vector<size_t> to a Python list
boost::python::list createList(std::vector<size_t> items) {
  // Create a list to populate
  boost::python::list dataAsList;

  // Store the data
  dataAsList.append(object(handle<>(
      Converters::VectorToNDArray<size_t, Converters::WrapReadOnly>()(items))));

  return dataAsList;
}

// Helper function to call the detectorsInSubtree method
boost::python::list createListForDetectorsInSubtree(const ComponentInfo &self,
                                                    const size_t index) {
  return createList(self.detectorsInSubtree(index));
}

// Helper function to call the componentsInSubtree method
boost::python::list createListForComponentsInSubtree(const ComponentInfo &self,
                                                     const size_t index) {
  return createList(self.componentsInSubtree(index));
}

// Helper function to call the children method
boost::python::list createListForComponentChildren(const ComponentInfo &self,
                                                   const size_t index) {
  return createList(self.children(index));
}

// Function pointers to help resolve ambiguity
Mantid::Kernel::V3D (ComponentInfo::*position)(const size_t) const =
    &ComponentInfo::position;

Mantid::Kernel::Quat (ComponentInfo::*rotation)(const size_t) const =
    &ComponentInfo::rotation;

void (ComponentInfo::*setPosition)(const size_t, const Mantid::Kernel::V3D &) =
    &ComponentInfo::setPosition;

void (ComponentInfo::*setRotation)(const size_t, const Mantid::Kernel::Quat &) =
    &ComponentInfo::setRotation;

// Export ComponentInfo
void export_ComponentInfo() {

  class_<ComponentInfo, boost::noncopyable>("ComponentInfo", no_init)

      .def("__len__", &ComponentInfo::size, arg("self"),
           "Returns the number of components.")

      .def("size", &ComponentInfo::size, arg("self"),
           "Returns the number of components.")

      .def("isDetector", &ComponentInfo::isDetector,
           (arg("self"), arg("index")),
           "Checks if the component is a detector.")

      .def("detectorsInSubtree", createListForDetectorsInSubtree,
           (arg("self"), arg("index")),
           "Returns a list of detectors in the subtree for the component "
           "identified by 'index'.")

      .def("componentsInSubtree", createListForComponentsInSubtree,
           (arg("self"), arg("index")),
           "Returns a list of components in the subtree for the component "
           "identified by 'index'.")

      .def("position", position, (arg("self"), arg("index")),
           "Returns the absolute position of the component identified by "
           "'index'.")

      .def("rotation", rotation, (arg("self"), arg("index")),
           "Returns the absolute rotation of the component identified by "
           "'index'.")

      .def("relativePosition", &ComponentInfo::relativePosition,
           (arg("self"), arg("index")),
           "Returns the absolute relative position of the component identified "
           "by 'index'.")

      .def("relativeRotation", &ComponentInfo::relativeRotation,
           (arg("self"), arg("index")),
           "Returns the absolute relative rotation of the component identified "
           "by 'index'.")

      .def("setPosition", setPosition,
           (arg("self"), arg("index"), arg("newPosition")),
           "Set the absolute position of the component identified by 'index'.")

      .def("setRotation", setRotation,
           (arg("self"), arg("index"), arg("newRotation")),
           "Set the absolute rotation of the component identified by 'index'.")

      .def("hasSource", &ComponentInfo::hasSource, arg("self"),
           "Returns True if a source is present.")

      .def("hasSample", &ComponentInfo::hasSample, arg("self"),
           "Returns True if a sample is present.")

      .def("source", &ComponentInfo::source, arg("self"),
           "Returns the source component.")

      .def("sample", &ComponentInfo::source, arg("self"),
           "Returns the sample component.")

      .def("sourcePosition", &ComponentInfo::sourcePosition, arg("self"),
           "Returns the source position.")

      .def("samplePosition", &ComponentInfo::samplePosition, arg("self"),
           "Returns the sample position.")

      .def("hasParent", &ComponentInfo::hasParent, (arg("self"), arg("index")),
           "Returns True only if the component identified by 'index' has a "
           "parent component.")

      .def("parent", &ComponentInfo::parent, (arg("self"), arg("index")),
           "Returns the parent component of the component identified by "
           "'index'.")

      .def("children", createListForComponentChildren,
           (arg("self"), arg("index")),
           "Returns a list of child components for the component identified by "
           "'index'.")

      .def("name", &ComponentInfo::name, (arg("self"), arg("index")),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the component identified by 'index'.");
}
