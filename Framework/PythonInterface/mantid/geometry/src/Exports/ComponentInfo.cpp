// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidBeamline/PixelGridComponent.h"
#include "MantidGeometry/Instrument/SolidAngleParams.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/geometry/ComponentInfoPythonIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>

using Mantid::Beamline::ComponentType;
using Mantid::Beamline::PixelGridComponent;
using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::SolidAngleParams;
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

// Accept a bare observer position so this reads as a drop-in for the legacy
// IDetector::solidAngle(observer), constructing SolidAngleParams internally.
double solidAngle(const ComponentInfo &self, const size_t index, const V3D &observer) {
  return self.solidAngle(index, SolidAngleParams(observer));
}

// Flat accessors for the grid metadata of a Rectangular/Grid bank. The
// Beamline::PixelGridComponent aggregate is an implementation detail of the
// Geometry layer and is deliberately not exported to Python; one member is
// fetched per call.
template <auto Field> auto pixelGridField(ComponentInfo const &self, size_t const componentIndex) {
  return self.pixelGridComponent(componentIndex).*Field;
}

// Axis fill order ('x'/'y'/'z' permutation) as a 3-character string, since
// std::array<char, 3> has no automatic boost::python converter.
std::string pixelGridIdFillOrder(ComponentInfo const &self, size_t const componentIndex) {
  auto const order = self.pixelGridComponent(componentIndex).idFillOrder;
  return std::string(order.begin(), order.end());
}

// The C++ add*() methods take the description as an optional pointer. Python passes a
// string, with the empty default standing in for "no description", since a parameter
// description is only ever set to something non-empty.
const std::string *descriptionOrNull(const std::string &description) {
  return description.empty() ? nullptr : &description;
}

// Named parameters are usually the instrument-level ones held by the root component, so
// 'index' is optional and None stands in for root(). A boost::python default cannot be
// computed from 'self', so it is resolved here rather than in the keyword list.
size_t indexOrRoot(const ComponentInfo &self, const object &componentIndex) {
  if (componentIndex.is_none()) {
    return self.root();
  }
  const extract<size_t> index(componentIndex);
  if (!index.check()) {
    PyErr_SetString(PyExc_TypeError, "'index' must be a component index, or None for the root component.");
    throw_error_already_set();
  }
  return index();
}

// Reorders Python's (name, index, recursive) to the C++ (index, name, recursive).
template <auto Method, typename... Leading>
auto readParameter(const ComponentInfo &self, Leading... leading, const object &componentIndex, bool recursive) {
  return (self.*Method)(indexOrRoot(self, componentIndex), leading..., recursive);
}

double getFittingParameter(const ComponentInfo &self, const std::string &name, double xvalue,
                           const object &componentIndex) {
  return self.getFittingParameter(indexOrRoot(self, componentIndex), name, xvalue);
}

void addParameter(ComponentInfo &self, const std::string &type, const std::string &name, const std::string &value,
                  const object &componentIndex, const std::string &description, const std::string &visible) {
  self.addParameter(indexOrRoot(self, componentIndex), type, name, value, descriptionOrNull(description), visible);
}

void addDouble(ComponentInfo &self, const std::string &name, double value, const object &componentIndex,
               const std::string &description, const std::string &visible) {
  self.addDouble(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description), visible);
}

void addInt(ComponentInfo &self, const std::string &name, int value, const object &componentIndex,
            const std::string &description, const std::string &visible) {
  self.addInt(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description), visible);
}

void addBool(ComponentInfo &self, const std::string &name, bool value, const object &componentIndex,
             const std::string &description, const std::string &visible) {
  self.addBool(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description), visible);
}

void addString(ComponentInfo &self, const std::string &name, const std::string &value, const object &componentIndex,
               const std::string &description, const std::string &visible) {
  self.addString(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description), visible);
}

void addV3D(ComponentInfo &self, const std::string &name, const V3D &value, const object &componentIndex,
            const std::string &description) {
  self.addV3D(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description));
}

void addQuat(ComponentInfo &self, const std::string &name, const Quat &value, const object &componentIndex,
             const std::string &description) {
  self.addQuat(indexOrRoot(self, componentIndex), name, value, descriptionOrNull(description));
}

void addFittingParameter(ComponentInfo &self, const std::string &name, const std::string &fittingFunction,
                         const std::string &value, const object &componentIndex, const std::string &description,
                         const std::string &visible) {
  self.addFittingParameter(indexOrRoot(self, componentIndex), name, fittingFunction, value,
                           descriptionOrNull(description), visible);
}

void clearParameter(ComponentInfo &self, const std::string &name, const object &componentIndex) {
  self.clearParameter(indexOrRoot(self, componentIndex), name);
}

dict shapeToComponentIndices(const ComponentInfo &componentInfo) {
  dict result;
  const auto shapeMap = componentInfo.shapeToComponentIndices();
  for (const auto &shape : shapeMap) {
    const auto csgObject = std::dynamic_pointer_cast<const Mantid::Geometry::CSGObject>(shape.first);
    if (csgObject != nullptr) {
      result[csgObject->getShapeXML()] = shape.second;
    }
  }
  return result;
}

} // namespace

// Function pointers to help resolve ambiguity
Mantid::Kernel::V3D (ComponentInfo::*position)(const size_t) const = &ComponentInfo::position;

Mantid::Kernel::Quat (ComponentInfo::*rotation)(const size_t) const = &ComponentInfo::rotation;

void (ComponentInfo::*setPosition)(const size_t, const Mantid::Kernel::V3D &) = &ComponentInfo::setPosition;

void (ComponentInfo::*setRotation)(const size_t, const Mantid::Kernel::Quat &) = &ComponentInfo::setRotation;

// Export ComponentInfo
void export_ComponentInfo() {
  enum_<ComponentType>("ComponentType")
      .value("Generic", ComponentType::Generic)
      .value("Infinite", ComponentType::Infinite)
      .value("Grid", ComponentType::Grid)
      .value("Rectangular", ComponentType::Rectangular)
      .value("Structured", ComponentType::Structured)
      .value("Unstructured", ComponentType::Unstructured)
      .value("Detector", ComponentType::Detector)
      .value("OutlineComposite", ComponentType::OutlineComposite);

  class_<ComponentInfo, boost::noncopyable>("ComponentInfo", no_init)

      .def("__iter__", make_pyiterator)

      .def("__len__", &ComponentInfo::size, arg("self"), "Returns the number of components.")

      .def("size", &ComponentInfo::size, arg("self"), "Returns the number of components.")

      .def("isDetector", &ComponentInfo::isDetector, (arg("self"), arg("index")),
           "Checks if the component is a detector.")

      .def("detectorsInSubtree", &ComponentInfo::detectorsInSubtree, return_value_policy<VectorToNumpy>(),
           (arg("self"), arg("index")),
           "Returns a list of detectors in the subtree for the component "
           "identified by 'index'.")

      .def("componentsInSubtree", &ComponentInfo::componentsInSubtree, return_value_policy<VectorToNumpy>(),
           (arg("self"), arg("index")),
           "Returns a list of components in the subtree for the component "
           "identified by 'index'.")

      .def("position", position, (arg("self"), arg("index")),
           "Returns the absolute position of the component identified by "
           "'index'.")

      .def("rotation", rotation, (arg("self"), arg("index")),
           "Returns the absolute rotation of the component identified by "
           "'index'.")

      .def("relativePosition", &ComponentInfo::relativePosition, (arg("self"), arg("index")),
           "Returns the absolute relative position of the component identified "
           "by 'index'.")

      .def("relativeRotation", &ComponentInfo::relativeRotation, (arg("self"), arg("index")),
           "Returns the absolute relative rotation of the component identified "
           "by 'index'.")

      .def("setPosition", setPosition, (arg("self"), arg("index"), arg("newPosition")),
           "Set the absolute position of the component identified by 'index'.")

      .def("setRotation", setRotation, (arg("self"), arg("index"), arg("newRotation")),
           "Set the absolute rotation of the component identified by 'index'.")

      .def("hasSource", &ComponentInfo::hasSource, arg("self"), "Returns True if a source is present.")

      .def("hasEquivalentSource", &ComponentInfo::hasEquivalentSource, arg("self"), arg("other"),
           "Returns True is both beamlines either lack a Source or "
           "have a Source at the same position.")

      .def("hasSample", &ComponentInfo::hasSample, arg("self"), "Returns True if a sample is present.")

      .def("hasEquivalentSample", &ComponentInfo::hasEquivalentSample, arg("self"), arg("other"),
           "Returns True is both beamlines either lack a Sample or "
           "have a Sample at the same position.")

      .def("source", &ComponentInfo::source, arg("self"), "Returns the source component index.")

      .def("sample", &ComponentInfo::sample, arg("self"), "Returns the sample component index.")

      .def("sourcePosition", &ComponentInfo::sourcePosition, arg("self"), "Returns the source position.")

      .def("samplePosition", &ComponentInfo::samplePosition, arg("self"), "Returns the sample position.")

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

      .def("name", &ComponentInfo::name, (arg("self"), arg("index")), return_value_policy<copy_const_reference>(),
           "Returns the name of the component identified by 'index'.")

      .def("l1", &ComponentInfo::l1, arg("self"), "Returns the l1 value.")

      .def("scaleFactor", &ComponentInfo::scaleFactor, (arg("self"), arg("index")),
           "Returns the scale factor for the component identified by 'index'.")

      .def("setScaleFactor", &ComponentInfo::setScaleFactor, (arg("self"), arg("index"), arg("scaleFactor")),
           "Set the scale factor of the component identifed by 'index'.")

      .def("hasValidShape", &ComponentInfo::hasValidShape, (arg("self"), arg("index")),
           "Returns True if the component identified by 'index' has a valid "
           "shape.")

      .def("shape", &ComponentInfo::shape, (arg("self"), arg("index")),
           return_value_policy<reference_existing_object>(),
           "Returns the shape of the component identified by 'index'.")

      .def("solidAngle", &solidAngle, (arg("self"), arg("index"), arg("observer")),
           "Returns the solid angle of the component identified by 'index' as "
           "seen from the observer position.")

      .def("componentType", &ComponentInfo::componentType, (arg("self"), arg("index")),
           "Returns the ComponentType of the component identified by 'index'.")

      .def("isGridDetector", &ComponentInfo::isGridDetector, (arg("self"), arg("index")),
           "Returns True if the component identified by 'index' is a Rectangular or Grid "
           "bank. Every pixelGrid* accessor requires this to be True.")

      .def("pixelGridNX", &pixelGridField<&PixelGridComponent::nX>, (arg("self"), arg("index")),
           "Returns the number of pixels in the X (horizontal) direction of the "
           "Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridNY", &pixelGridField<&PixelGridComponent::nY>, (arg("self"), arg("index")),
           "Returns the number of pixels in the Y (vertical) direction of the "
           "Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridNZ", &pixelGridField<&PixelGridComponent::nZ>, (arg("self"), arg("index")),
           "Returns the number of pixels in the Z (usually beam) direction of the "
           "Rectangular/Grid bank identified by 'index', or 0 for a 2D (rectangular) "
           "bank. Raises RuntimeError if the component is not a Rectangular or Grid bank.")

      .def("pixelGridXStart", &pixelGridField<&PixelGridComponent::xStart>, (arg("self"), arg("index")),
           "Returns the X position of pixel (0, 0, 0), in the local (unrotated, unscaled) "
           "frame of the Rectangular/Grid bank identified by 'index'. Raises RuntimeError "
           "if the component is not a Rectangular or Grid bank.")

      .def("pixelGridYStart", &pixelGridField<&PixelGridComponent::yStart>, (arg("self"), arg("index")),
           "Returns the Y position of pixel (0, 0, 0), in the local (unrotated, unscaled) "
           "frame of the Rectangular/Grid bank identified by 'index'. Raises RuntimeError "
           "if the component is not a Rectangular or Grid bank.")

      .def("pixelGridZStart", &pixelGridField<&PixelGridComponent::zStart>, (arg("self"), arg("index")),
           "Returns the Z position of pixel (0, 0, 0), in the local (unrotated, unscaled) "
           "frame of the Rectangular/Grid bank identified by 'index'. Raises RuntimeError "
           "if the component is not a Rectangular or Grid bank.")

      .def("pixelGridXStep", &pixelGridField<&PixelGridComponent::xStep>, (arg("self"), arg("index")),
           "Returns the step size between neighbouring pixels along X for the "
           "Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridYStep", &pixelGridField<&PixelGridComponent::yStep>, (arg("self"), arg("index")),
           "Returns the step size between neighbouring pixels along Y for the "
           "Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridZStep", &pixelGridField<&PixelGridComponent::zStep>, (arg("self"), arg("index")),
           "Returns the step size between neighbouring pixels along Z for the "
           "Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridIdStart", &pixelGridField<&PixelGridComponent::idStart>, (arg("self"), arg("index")),
           "Returns the detector ID of the first pixel of the Rectangular/Grid bank "
           "identified by 'index'. Raises RuntimeError if the component is not a "
           "Rectangular or Grid bank.")

      .def("pixelGridIdStep", &pixelGridField<&PixelGridComponent::idStep>, (arg("self"), arg("index")),
           "Returns the detector ID step between pixels along the first-filled axis of "
           "the Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridIdStepByRow", &pixelGridField<&PixelGridComponent::idStepByRow>, (arg("self"), arg("index")),
           "Returns the detector ID step between rows along the second-filled axis of "
           "the Rectangular/Grid bank identified by 'index'. Raises RuntimeError if the "
           "component is not a Rectangular or Grid bank.")

      .def("pixelGridMinDetectorID", &pixelGridField<&PixelGridComponent::minDetectorID>, (arg("self"), arg("index")),
           "Returns the minimum detector ID in the Rectangular/Grid bank identified by "
           "'index'. Raises RuntimeError if the component is not a Rectangular or Grid bank.")

      .def("pixelGridMaxDetectorID", &pixelGridField<&PixelGridComponent::maxDetectorID>, (arg("self"), arg("index")),
           "Returns the maximum detector ID in the Rectangular/Grid bank identified by "
           "'index'. Raises RuntimeError if the component is not a Rectangular or Grid bank.")

      .def("pixelGridIdFillOrder", &pixelGridIdFillOrder, (arg("self"), arg("index")),
           "Returns the axis order in which detector IDs are filled for the "
           "Rectangular/Grid bank identified by 'index', as a 3-character string "
           "permutation of 'x', 'y' and 'z'. Raises RuntimeError if the component is not "
           "a Rectangular or Grid bank.")

      .def("detectorIndexAtXYZ", &ComponentInfo::detectorIndexAtXYZ,
           (arg("self"), arg("index"), arg("x"), arg("y"), arg("z")),
           "Returns the detector index of the pixel at (x, y, z) within the "
           "Rectangular/Grid bank identified by 'index'.")

      .def("indexOfAny", &ComponentInfo::indexOfAny, (arg("self"), arg("name")),
           "Returns the index of any component matching name. Raises "
           "ValueError if name not found")

      .def("uniqueName", &ComponentInfo::uniqueName, (arg("self"), arg("name")),
           "Returns True if the name is a unique single occurance. Zero occurances yields False.")

      .def("root", &ComponentInfo::root, arg("self"), "Returns the index of the root component")
      .def("getMemorySize", &ComponentInfo::getMemorySize, arg("self"),
           "Return the memory footprint of the component info in bytes.")
      .def("shapeToComponentIndices", &shapeToComponentIndices, arg("self"),
           "Returns a mapping of shapes to the indices of components with that shape.")

      .def("fullName", &ComponentInfo::fullName, (arg("self"), arg("index")),
           "Returns the fully-qualified name of the component identified by 'index', "
           "e.g. 'instrument/bank1/pixel3'.")

      .def("indexOfFullName", &ComponentInfo::indexOfFullName, (arg("self"), arg("fullName")),
           "Returns the index of the component with this fully-qualified name, or "
           "ComponentInfo.invalidIndex if there is none.")

      // Named parameters. The read accessors mirror the legacy component API, including the
      // 'empty sequence if absent' convention and the 'recursive' flag, so that migrating a
      // call site is a rename. 'index' defaults to the root component, which holds the
      // instrument-level parameters the legacy Instrument methods looked up.
      .def("hasParameter", &readParameter<&ComponentInfo::hasParameter, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns True if the component identified by 'index' (the root component by "
           "default) has a parameter of this name.")

      .def("getParameterNames", &readParameter<&ComponentInfo::getParameterNames>,
           (arg("self"), arg("index") = object(), arg("recursive") = true),
           "Returns the names of the parameters on the component identified by 'index' "
           "(the root component by default).")

      .def("getNumberParameter", &readParameter<&ComponentInfo::getNumberParameter, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns the named double parameter of the component identified by 'index' "
           "(the root component by default), or an empty sequence if it is unset.")

      .def("getIntParameter", &readParameter<&ComponentInfo::getIntParameter, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns the named integer parameter of the component identified by 'index' "
           "(the root component by default), or an empty sequence if it is unset.")

      .def("getBoolParameter", &readParameter<&ComponentInfo::getBoolParameter, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns the named boolean parameter of the component identified by 'index' "
           "(the root component by default), or an empty sequence if it is unset.")

      .def("getStringParameter", &readParameter<&ComponentInfo::getStringParameter, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns the named string parameter of the component identified by 'index' "
           "(the root component by default), or an empty sequence if it is unset.")

      .def("getParameterType", &readParameter<&ComponentInfo::getParameterType, const std::string &>,
           (arg("self"), arg("name"), arg("index") = object(), arg("recursive") = true),
           "Returns the type of the named parameter of the component identified by 'index' "
           "(the root component by default), or an empty string if it is unset.")

      .def("getFittingParameter", &getFittingParameter,
           (arg("self"), arg("name"), arg("xvalue"), arg("index") = object()),
           "Returns the named fitting parameter of the component identified by 'index' (the "
           "root component by default), evaluated at 'xvalue' from its look-up table or formula.")

      .def("addParameter", &addParameter,
           (arg("self"), arg("type"), arg("name"), arg("value"), arg("index") = object(), arg("description") = "",
            arg("visible") = "true"),
           "Adds or replaces the named parameter of this type, given its value as a string, "
           "on the component identified by 'index' (the root component by default).")

      .def("addDouble", &addDouble,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = "",
            arg("visible") = "true"),
           "Adds or replaces a named double parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addInt", &addInt,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = "",
            arg("visible") = "true"),
           "Adds or replaces a named integer parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addBool", &addBool,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = "",
            arg("visible") = "true"),
           "Adds or replaces a named boolean parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addString", &addString,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = "",
            arg("visible") = "true"),
           "Adds or replaces a named string parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addV3D", &addV3D,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = ""),
           "Adds or replaces a named V3D parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addQuat", &addQuat,
           (arg("self"), arg("name"), arg("value"), arg("index") = object(), arg("description") = ""),
           "Adds or replaces a named Quat parameter on the component identified by 'index' "
           "(the root component by default).")

      .def("addFittingParameter", &addFittingParameter,
           (arg("self"), arg("name"), arg("fittingFunction"), arg("value"), arg("index") = object(),
            arg("description") = "", arg("visible") = "true"),
           "Adds a named fitting parameter, given its value as a string, on the component "
           "identified by 'index' (the root component by default).")

      .def("clearParameter", &clearParameter, (arg("self"), arg("name"), arg("index") = object()),
           "Removes every parameter of this name from the component identified by 'index' "
           "(the root component by default).");

  // The sentinel returned by indexOfFullName() for a name that matches no component.
  scope().attr("ComponentInfo").attr("invalidIndex") = ComponentInfo::invalidIndex;
}
