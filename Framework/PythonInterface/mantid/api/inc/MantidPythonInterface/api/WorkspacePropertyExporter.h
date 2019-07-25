// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_
#define MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include <boost/python/args.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/register_ptr_to_python.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * A helper struct to export WorkspaceProperty<> types to Python. It also
 * exports a new PropertyWithValue<WorkspaceType> type as this is required to
 * for the base class.
 */
template <typename WorkspaceType> struct WorkspacePropertyExporter {
  /// The export type
  using TypedWorkspaceProperty = Mantid::API::WorkspaceProperty<WorkspaceType>;
  /// Shared pointer to Worksapce type
  using WorkspaceType_sptr = boost::shared_ptr<WorkspaceType>;

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param wsName :: A default value
   * @param direction :: The direction, @see Direction enum
   * @param validator :: A pointer validator object
   */
  static TypedWorkspaceProperty *createPropertyWithValidator(
      const std::string &name, const std::string &wsName,
      const unsigned int direction, Kernel::IValidator *validator) {
    return new TypedWorkspaceProperty(name, wsName, direction,
                                      validator->clone());
  }

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param wsName :: A default value
   * @param direction :: The direction, @see Direction enum
   * @param optional :: If true then the workspace is optional
   * @param validator :: A pointer validator object
   */
  static TypedWorkspaceProperty *createPropertyWithOptionalFlag(
      const std::string &name, const std::string &wsName,
      const unsigned int direction, API::PropertyMode::Type optional,
      Kernel::IValidator *validator) {
    return new TypedWorkspaceProperty(name, wsName, direction, optional,
                                      validator->clone());
  }

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param wsName :: A default value
   * @param direction :: The direction, @see Direction enum
   * @param validator :: A pointer validator object
   * @param optional :: If true then the workspace is optional
   * @param locking :: If true then the workspace will be locked before running
   * an algorithm
   */
  static TypedWorkspaceProperty *createPropertyWithLockFlag(
      const std::string &name, const std::string &wsName,
      const unsigned int direction, API::PropertyMode::Type optional,
      API::LockMode::Type locking, Kernel::IValidator *validator) {
    return new TypedWorkspaceProperty(name, wsName, direction, optional,
                                      locking, validator->clone());
  }

  /**
   * Ensure the stored type is always a Workspace_sptr
   * This allows a reference to a Workspace_sptr to be used withh
   * boost::python::extract
   */
  static Mantid::API::Workspace_sptr value(const TypedWorkspaceProperty &self) {
    return self.operator()();
  }

  /**
   * Defines the necessary exports for a WorkspaceProperty<WorkspaceType>. This
   * includes a
   * PropertyWithValue<WorkspaceType_sptr> whose name is formed by appending
   * "PropertyWithValue"
   * to the given class name
   * @param pythonClassName :: The name of the class in python
   */
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using Mantid::API::IWorkspaceProperty;
    using Mantid::Kernel::PropertyWithValue;

    std::string basePropName =
        std::string(pythonClassName) + "PropertyWithValue";
    PropertyWithValueExporter<WorkspaceType_sptr>::define(basePropName.c_str());
    register_ptr_to_python<TypedWorkspaceProperty *>();

    class_<TypedWorkspaceProperty,
           bases<PropertyWithValue<WorkspaceType_sptr>, IWorkspaceProperty>,
           boost::noncopyable>(pythonClassName, no_init)
        .def(init<const std::string &, const std::string &, const unsigned int>(
            args("name", "defaultValue", "direction")))
        .def(init<const std::string &, const std::string &, const unsigned int,
                  API::PropertyMode::Type>(
            args("name", "defaultValue", "direction", "optional")))
        .def(init<const std::string &, const std::string &, const unsigned int,
                  API::PropertyMode::Type, API::LockMode::Type>(
            args("name", "defaultValue", "direction", "optional", "locking")))
        // These variants require the validator object to be cloned
        .def("__init__", make_constructor(&createPropertyWithValidator,
                                          default_call_policies(),
                                          (arg("name"), arg("defaultValue"),
                                           arg("direction"), arg("validator"))))
        .def("__init__",
             make_constructor(&createPropertyWithOptionalFlag,
                              default_call_policies(),
                              args("name", "defaultValue", "direction",
                                   "optional", "validator")))
        .def("__init__",
             make_constructor(&createPropertyWithLockFlag,
                              default_call_policies(),
                              args("name", "defaultValue", "direction",
                                   "optional", "locking", "validator")))
        .def("isOptional", &TypedWorkspaceProperty::isOptional, arg("self"),
             "Returns true if the property has been marked as optional")

        .add_property("value", &value);
  }
};
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_ */
