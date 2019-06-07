// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/api/ComponentInfoPythonIterator.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::ComponentInfo;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using Mantid::PythonInterface::ComponentInfoPythonIterator;
using namespace Mantid::PythonInterface::Converters;
using namespace Mantid::PythonInterface::Policies;
using namespace boost::python;

namespace {
ComponentInfoPythonIterator make_pyiterator(ComponentInfo &componentInfo) {
  return ComponentInfoPythonIterator(componentInfo);
}
} // namespace

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

      .def("__iter__", make_pyiterator)

      .def("__len__", &ComponentInfo::size, arg("self"),
           "Returns the number of components.")

      .def("size", &ComponentInfo::size, arg("self"),
           "Returns the number of components.")

      .def("isDetector", &ComponentInfo::isDetector,
           (arg("self"), arg("index")),
           "Checks if the component is a detector.")

      .def("detectorsInSubtree", &ComponentInfo::detectorsInSubtree,
           return_value_policy<VectorToNumpy>(), (arg("self"), arg("index")),
           "Returns a list of detectors in the subtree for the component "
           "identified by 'index'.")

      .def("componentsInSubtree", &ComponentInfo::componentsInSubtree,
           return_value_policy<VectorToNumpy>(), (arg("self"), arg("index")),
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
           "Returns the source component index.")

      .def("sample", &ComponentInfo::sample, arg("self"),
           "Returns the sample component index.")

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

      .def("children", &ComponentInfo::children, (arg("self"), arg("index")),
           return_value_policy<VectorRefToNumpy<WrapReadOnly>>(),
           "Returns a list of child components for the component identified by "
           "'index'.")

      .def("name", &ComponentInfo::name, (arg("self"), arg("index")),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the component identified by 'index'.")

      .def("l1", &ComponentInfo::l1, arg("self"), "Returns the l1 value.")

      .def("scaleFactor", &ComponentInfo::scaleFactor,
           (arg("self"), arg("index")),
           "Returns the scale factor for the component identified by 'index'.")

      .def("setScaleFactor", &ComponentInfo::setScaleFactor,
           (arg("self"), arg("index"), arg("scaleFactor")),
           "Set the scale factor of the component identifed by 'index'.")

      .def("hasValidShape", &ComponentInfo::hasValidShape,
           (arg("self"), arg("index")),
           "Returns True if the component identified by 'index' has a valid "
           "shape.")

      .def("shape", &ComponentInfo::shape, (arg("self"), arg("index")),
           return_value_policy<reference_existing_object>(),
           "Returns the shape of the component identified by 'index'.")

      .def("indexOfAny", &ComponentInfo::indexOfAny, (arg("self"), arg("name")),
           "Returns the index of any component matching name. Raises "
           "ValueError if name not found")

      .def("root", &ComponentInfo::root, arg("self"),
           "Returns the index of the root component");
}
