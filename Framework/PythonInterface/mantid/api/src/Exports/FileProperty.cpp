// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/IsNone.h"

#include <boost/make_shared.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::FileProperty;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::Converters::PySequenceToVector;
namespace bpl = boost::python;

void export_ActionEnum() {
  bpl::enum_<FileProperty::FileAction>("FileAction")
      .value("Save", FileProperty::Save)
      .value("OptionalSave", FileProperty::OptionalSave)
      .value("Load", FileProperty::Load)
      .value("OptionalLoad", FileProperty::OptionalLoad)
      .value("Directory", FileProperty::Directory)
      .value("OptionalDirectory", FileProperty::OptionalDirectory);
}

namespace {
/**
 * The FileProperty constructor can take a list of extensions but we want users
 * to be
 * able to pass in a python list so we need a proxy function to act as a
 * constructor
 * @param name :: The name of the property
 * @param defaultValue :: A default value
 * @param action :: A file action defined by FileProperty::FileAction
 * @param extensions :: A list of possible extensions (default = [])
 * @param The direction of the property (default = input)
 */
FileProperty *
createFileProperty(const std::string &name, const std::string &defaultValue,
                   unsigned int action,
                   const bpl::object &extensions = bpl::object(),
                   unsigned direction = Mantid::Kernel::Direction::Input) {
  std::vector<std::string> extsAsVector;
  if (!Mantid::PythonInterface::isNone(extensions)) {
    bpl::extract<std::string> extractor(extensions);
    if (extractor.check()) {
      extsAsVector = std::vector<std::string>(1, extractor());
    } else {
      extsAsVector = PySequenceToVector<std::string>(extensions)();
    }
  }
  return new FileProperty(name, defaultValue, action, extsAsVector, direction);
}
} // namespace

void export_FileProperty() {
  bpl::class_<FileProperty, bpl::bases<PropertyWithValue<std::string>>,
              boost::noncopyable>("FileProperty", bpl::no_init)
      .def("__init__",
           bpl::make_constructor(
               &createFileProperty, bpl::default_call_policies(),
               (bpl::arg("name"), bpl::arg("defaultValue"), bpl::arg("action"),
                bpl::arg("extensions") = bpl::object(),
                bpl::arg("direction") = Mantid::Kernel::Direction::Input)));
}
