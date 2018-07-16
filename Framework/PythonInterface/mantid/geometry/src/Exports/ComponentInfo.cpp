#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::ComponentInfo;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using namespace boost::python;

// Helper function for converting std::vector<size_t> to a Python list
boost::python::list createDetectorsInSubtreeList(std::vector<size_t> items) {
  // Create the result list
  boost::python::list result;

  // Populate the Python list
  std::vector<size_t>::iterator it;
  for (it = items.begin(); it != items.end(); ++it) {
    result.append(*it);
  }

  return result;
}

// Helper function to call the detectorsInSubtree method
boost::python::list createListForDetectorsInSubtree(const ComponentInfo &self,
                                                    const size_t index) {
  return createDetectorsInSubtreeList(self.detectorsInSubtree(index));
}

// Helper function to call the componentsInSubtree method
boost::python::list createListForComponentsInSubtree(const ComponentInfo &self,
                                                     const size_t index) {
  return createDetectorsInSubtreeList(self.componentsInSubtree(index));
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

void export_ComponentInfo() {
  // WARNING ComponentInfo is work in progress and not ready for exposing more
  // of its functionality to Python, and should not yet be used in user scripts.
  // DO NOT ADD EXPORTS TO OTHER METHODS without contacting the team working on
  // Instrument-2.0.
  class_<ComponentInfo, boost::noncopyable>("ComponentInfo", no_init)
      .def("detectorsInSubtree", createListForDetectorsInSubtree,
           (arg("self"), arg("index")),
           "Returns a list of detectors in the subtree.")

      .def("componentsInSubtree", createListForComponentsInSubtree,
           (arg("self"), arg("index")),
           "Returns a list of components in the subtree.")

      .def("size", &ComponentInfo::size, arg("self"),
           "Returns the number of components.")

      .def("isDetector", &ComponentInfo::isDetector,
           (arg("self"), arg("index")),
           "Checks if the component is a detector.")

      .def("hasDetectorInfo", &ComponentInfo::hasDetectorInfo, arg("self"),
           "Checks if the component has a DetectorInfo object.")

      .def("position", position, (arg("self"), arg("index")),
           "Returns the absolute position of the component at the given index.")

      .def("rotation", rotation, (arg("self"), arg("index")),
           "Returns the absolute rotation of the component at the given index.")

      .def("relativePosition", &ComponentInfo::relativePosition,
           (arg("self"), arg("index")),
           "Returns the absolute relative position of the component at the "
           "given index.")

      .def("relativeRotation", &ComponentInfo::relativeRotation,
           (arg("self"), arg("index")),
           "Returns the absolute relative rotation of the component at the "
           "given index.")

      .def("setPosition", setPosition,
           (arg("self"), arg("index"), arg("newPosition")),
           "Set the absolute position of the component at the given index.")

      .def("setRotation", setRotation,
           (arg("self"), arg("index"), arg("newRotation")),
           "Set the absolute rotation of the component at the given index.")

      .def("hasSource", &ComponentInfo::hasSource, arg("self"),
           "Returns whether a source is present.")

      .def("hasSample", &ComponentInfo::hasSample, arg("self"),
           "Returns whether a sample is present.")

      .def("sourcePosition", &ComponentInfo::sourcePosition, arg("self"),
           "Returns the source position.")

      .def("samplePosition", &ComponentInfo::samplePosition, arg("self"),
           "Returns the sample position.")

      .def("name", &ComponentInfo::name, (arg("self"), arg("index")),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the component at the given index.");
}
