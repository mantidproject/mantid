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
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/geometry/ComponentInfoPythonIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/overloads.hpp>
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

void addParameter(ComponentInfo &self, const size_t componentIndex, const std::string &type, const std::string &name,
                  const std::string &value, const std::string &description = "", const std::string &visible = "true") {
  self.addParameter(componentIndex, type, name, value, descriptionOrNull(description), visible);
}

void addDouble(ComponentInfo &self, const size_t componentIndex, const std::string &name, double value,
               const std::string &description = "", const std::string &visible = "true") {
  self.addDouble(componentIndex, name, value, descriptionOrNull(description), visible);
}

void addInt(ComponentInfo &self, const size_t componentIndex, const std::string &name, int value,
            const std::string &description = "", const std::string &visible = "true") {
  self.addInt(componentIndex, name, value, descriptionOrNull(description), visible);
}

void addBool(ComponentInfo &self, const size_t componentIndex, const std::string &name, bool value,
             const std::string &description = "", const std::string &visible = "true") {
  self.addBool(componentIndex, name, value, descriptionOrNull(description), visible);
}

void addString(ComponentInfo &self, const size_t componentIndex, const std::string &name, const std::string &value,
               const std::string &description = "", const std::string &visible = "true") {
  self.addString(componentIndex, name, value, descriptionOrNull(description), visible);
}

void addV3D(ComponentInfo &self, const size_t componentIndex, const std::string &name, const V3D &value,
            const std::string &description = "") {
  self.addV3D(componentIndex, name, value, descriptionOrNull(description));
}

void addQuat(ComponentInfo &self, const size_t componentIndex, const std::string &name, const Quat &value,
             const std::string &description = "") {
  self.addQuat(componentIndex, name, value, descriptionOrNull(description));
}

void addFittingParameter(ComponentInfo &self, const size_t componentIndex, const std::string &name,
                         const std::string &fittingFunction, const std::string &value,
                         const std::string &description = "", const std::string &visible = "true") {
  self.addFittingParameter(componentIndex, name, fittingFunction, value, descriptionOrNull(description), visible);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
GNU_DIAG_OFF("conversion")

// Default arguments for the parameter accessors
// cppcheck-suppress unknownMacro
BOOST_PYTHON_FUNCTION_OVERLOADS(addParameterOverloads, addParameter, 5, 7)
BOOST_PYTHON_FUNCTION_OVERLOADS(addDoubleOverloads, addDouble, 4, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(addIntOverloads, addInt, 4, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(addBoolOverloads, addBool, 4, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(addStringOverloads, addString, 4, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(addV3DOverloads, addV3D, 4, 5)
BOOST_PYTHON_FUNCTION_OVERLOADS(addQuatOverloads, addQuat, 4, 5)
BOOST_PYTHON_FUNCTION_OVERLOADS(addFittingParameterOverloads, addFittingParameter, 5, 7)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(hasParameterOverloads, ComponentInfo::hasParameter, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getParameterNamesOverloads, ComponentInfo::getParameterNames, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getNumberParameterOverloads, ComponentInfo::getNumberParameter, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getIntParameterOverloads, ComponentInfo::getIntParameter, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getBoolParameterOverloads, ComponentInfo::getBoolParameter, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getStringParameterOverloads, ComponentInfo::getStringParameter, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getParameterTypeOverloads, ComponentInfo::getParameterType, 2, 3)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

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
      // call site is a rename once the component index is in hand.
      .def("hasParameter", &ComponentInfo::hasParameter,
           hasParameterOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                 "Returns True if the component identified by 'index' has a parameter "
                                 "of this name."))

      .def("getParameterNames", &ComponentInfo::getParameterNames,
           getParameterNamesOverloads((arg("self"), arg("index"), arg("recursive") = true),
                                      "Returns the names of the parameters on the component identified by "
                                      "'index'."))

      .def("getNumberParameter", &ComponentInfo::getNumberParameter,
           getNumberParameterOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                       "Returns the named double parameter of the component identified by "
                                       "'index', or an empty sequence if it is unset."))

      .def("getIntParameter", &ComponentInfo::getIntParameter,
           getIntParameterOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                    "Returns the named integer parameter of the component identified by "
                                    "'index', or an empty sequence if it is unset."))

      .def("getBoolParameter", &ComponentInfo::getBoolParameter,
           getBoolParameterOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                     "Returns the named boolean parameter of the component identified by "
                                     "'index', or an empty sequence if it is unset."))

      .def("getStringParameter", &ComponentInfo::getStringParameter,
           getStringParameterOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                       "Returns the named string parameter of the component identified by "
                                       "'index', or an empty sequence if it is unset."))

      .def("getParameterType", &ComponentInfo::getParameterType,
           getParameterTypeOverloads((arg("self"), arg("index"), arg("name"), arg("recursive") = true),
                                     "Returns the type of the named parameter of the component identified "
                                     "by 'index', or an empty string if it is unset."))

      .def("getFittingParameter", &ComponentInfo::getFittingParameter,
           (arg("self"), arg("index"), arg("name"), arg("xvalue")),
           "Returns the named fitting parameter of the component identified by 'index', "
           "evaluated at 'xvalue' from its look-up table or formula.")

      .def("addParameter", &addParameter,
           addParameterOverloads((arg("self"), arg("index"), arg("type"), arg("name"), arg("value"),
                                  arg("description") = "", arg("visible") = "true"),
                                 "Adds or replaces the named parameter of this type, given its value as a "
                                 "string, on the component identified by 'index'."))

      .def("addDouble", &addDouble,
           addDoubleOverloads(
               (arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = "", arg("visible") = "true"),
               "Adds or replaces a named double parameter on the component identified by "
               "'index'."))

      .def("addInt", &addInt,
           addIntOverloads(
               (arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = "", arg("visible") = "true"),
               "Adds or replaces a named integer parameter on the component identified by "
               "'index'."))

      .def("addBool", &addBool,
           addBoolOverloads(
               (arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = "", arg("visible") = "true"),
               "Adds or replaces a named boolean parameter on the component identified by "
               "'index'."))

      .def("addString", &addString,
           addStringOverloads(
               (arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = "", arg("visible") = "true"),
               "Adds or replaces a named string parameter on the component identified by "
               "'index'."))

      .def("addV3D", &addV3D,
           addV3DOverloads((arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = ""),
                           "Adds or replaces a named V3D parameter on the component identified by 'index'."))

      .def("addQuat", &addQuat,
           addQuatOverloads((arg("self"), arg("index"), arg("name"), arg("value"), arg("description") = ""),
                            "Adds or replaces a named Quat parameter on the component identified by "
                            "'index'."))

      .def("addFittingParameter", &addFittingParameter,
           addFittingParameterOverloads((arg("self"), arg("index"), arg("name"), arg("fittingFunction"), arg("value"),
                                         arg("description") = "", arg("visible") = "true"),
                                        "Adds a named fitting parameter, given its value as a string, on the "
                                        "component identified by 'index'."))

      .def("clearParameter", &ComponentInfo::clearParameter, (arg("self"), arg("index"), arg("name")),
           "Removes every parameter of this name from the component identified by 'index'.");

  // The sentinel returned by indexOfFullName() for a name that matches no component.
  scope().attr("ComponentInfo").attr("invalidIndex") = ComponentInfo::invalidIndex;
}
