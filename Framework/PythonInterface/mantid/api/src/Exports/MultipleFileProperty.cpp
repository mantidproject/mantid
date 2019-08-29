// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/IsNone.h"
#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/str.hpp>

using Mantid::API::FileProperty;
using Mantid::API::MultipleFileProperty;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::Converters::PySequenceToVector;
using Mantid::PythonInterface::PropertyWithValueExporter;
using namespace boost::python;

namespace {
/// The PropertyWithValue type
using HeldType = std::vector<std::vector<std::string>>;

/**
 * Converts the value from a MultipleFileProperty to a python object rather than
 * using a vector
 * @param self :: A reference to the calling object
 * @returns A string is there is only a single string in the Property's value,
 * and a list if there are multiple ones
 */
boost::python::list valueAsPyObject(MultipleFileProperty &self) {
  const HeldType &propValue = self();

  // Build a list of lists to mimic the behaviour of MultipleFileProperty
  boost::python::list fileList;
  for (const auto &filenames : propValue) {
    if (filenames.size() == 1) {
      fileList.append(filenames.front());
    } else {
      boost::python::list groupList;
      for (const auto &filename : filenames) {
        groupList.append(filename);
      }
      fileList.append(groupList);
    }
  }

  return fileList;
}

MultipleFileProperty *
createMultipleFilePropertyWithAction(const std::string &name,
                                     unsigned int action,
                                     const object &extensions = object()) {
  std::vector<std::string> extsAsVector;
  if (!Mantid::PythonInterface::isNone(extensions)) {
    extract<std::string> extractor(extensions);
    if (extractor.check()) {
      extsAsVector = std::vector<std::string>(1, extractor());
    } else {
      extsAsVector = PySequenceToVector<std::string>(extensions)();
    }
  }
  return new MultipleFileProperty(name, action, extsAsVector);
}

MultipleFileProperty *
createMultipleFileProperty(const std::string &name,
                           const object &extensions = object()) {
  return createMultipleFilePropertyWithAction(
      name, FileProperty::FileAction::Load, extensions);
}
} // namespace

void export_MultipleFileProperty() {
  using BaseClass = PropertyWithValue<HeldType>;
  PropertyWithValueExporter<HeldType>::define(
      "VectorVectorStringPropertyWithValue");

  class_<MultipleFileProperty, bases<BaseClass>, boost::noncopyable>(
      "MultipleFileProperty", no_init)
      .def("__init__", make_constructor(
                           &createMultipleFileProperty, default_call_policies(),
                           (arg("name"), arg("extensions") = object())))
      .def("__init__",
           make_constructor(
               &createMultipleFilePropertyWithAction, default_call_policies(),
               (arg("name"), arg("action"), arg("extensions") = object())))
      // Override the base class one to do something more appropriate
      .add_property("value", &valueAsPyObject);
}
