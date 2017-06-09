#ifndef MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYWITHINDEXMACRO_H_
#define MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYWITHINDEXMACRO_H_
/*
Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include <MantidIndexing/SpectrumIndexSet.h>
#include <boost/python/args.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/module.hpp>
#include <boost/python/object.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/tuple.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * A helper struct to export WorkspaceProperty<> types to Python. It also
 * exports a new PropertyWithValue<WorkspaceType> type as this is required to
 * for the base class.
 */
template <typename WorkspaceType> struct WorkspacePropertyWithIndexExporter {
  /// The export type
  typedef Mantid::API::WorkspacePropertyWithIndex<WorkspaceType>
      TypedWorkspacePropertyWithIndex;
  /// Shared pointer to Worksapce type
  typedef boost::shared_ptr<WorkspaceType> WorkspaceType_sptr;

  /**
   * Factory function to act as a constructor
   * rather than passing in the python owned object
   * @param name :: The name of the property
   */
  static TypedWorkspacePropertyWithIndex *
  createPropertyWithName(const std::string &name) {
    return new TypedWorkspacePropertyWithIndex(name);
  }

  /**
   * Factory function to act as a constructor
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param indexTypes :: Allowed indexTypes, @see IndexType enum
   */
  static TypedWorkspacePropertyWithIndex *
  createPropertyWithIndexType(const std::string &name,
                              const unsigned int indexTypes) {
    return new TypedWorkspacePropertyWithIndex(name, indexTypes);
  }

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param indexTypes :: Allowed indexTypes, @see IndexType enum
   * @param wsName :: A default value
   * @param validator :: A pointer validator object
   */
  static TypedWorkspacePropertyWithIndex *createPropertyWithValidator(
      const std::string &name, const unsigned int indexTypes,
      const std::string &wsName, Kernel::IValidator *validator) {
    return new TypedWorkspacePropertyWithIndex(name, indexTypes, wsName,
                                               validator->clone());
  }

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param indexTypes :: Allowed indexTypes, @see IndexType enum
   * @param wsName :: A default value
   * @param optional :: If true then the workspace is optional
   * @param validator :: A pointer validator object
   */
  static TypedWorkspacePropertyWithIndex *createPropertyWithOptionalFlag(
      const std::string &name, const unsigned int indexTypes,
      const std::string &wsName, API::PropertyMode::Type optional,
      Kernel::IValidator *validator) {
    return new TypedWorkspacePropertyWithIndex(name, indexTypes, wsName,
                                               optional, validator->clone());
  }

  /**
   * Factory function to act as a constructor so that the validator can be
   * cloned
   * rather than passing in the python owned object
   * @param name :: The name of the property
   * @param indexTypes :: Allowed indexTypes, see @IndexType enum
   * @param wsName :: A default value
   * @param validator :: A pointer validator object
   * @param optional :: If true then the workspace is optional
   * @param locking :: If true then the workspace will be locked before running
   * an algorithm
   */
  static TypedWorkspacePropertyWithIndex *createPropertyWithLockFlag(
      const std::string &name, const unsigned int indexTypes,
      const std::string &wsName, API::PropertyMode::Type optional,
      API::LockMode::Type locking, Kernel::IValidator *validator) {
    return new TypedWorkspacePropertyWithIndex(
        name, indexTypes, wsName, optional, locking, validator->clone());
  }

  /**
   * Ensure the stored type is always a Workspace_sptr
   * This allows a reference to a Workspace_sptr to be used withh
   * boost::python::extract
   */
  static Mantid::API::Workspace_sptr
  value(const TypedWorkspacePropertyWithIndex &self) {
    return self.operator()();
  }

  static const boost::python::tuple
  valueWithIndex(const TypedWorkspacePropertyWithIndex &self) {
    WorkspaceType_sptr wksp;
    Indexing::SpectrumIndexSet indices(0);
    std::tie(wksp, indices) =
        std::tuple<WorkspaceType_sptr &, Indexing::SpectrumIndexSet &>(self);

    boost::python::list indexList;
    for (auto i : indices)
      indexList.append<size_t>(i);

    return boost::python::make_tuple(wksp, indexList);
  }

  static void setIndexListString(TypedWorkspacePropertyWithIndex &self,
                                 const std::string &indexList) {
    self.mutableIndexListProperty().setValue(indexList);
  }

  static void setIndexListVector(TypedWorkspacePropertyWithIndex &self,
                                 boost::python::list &indexList) {
    std::vector<int> iList(boost::python::len(indexList));
    for (int i = 0; i < boost::python::len(indexList); i++)
      iList[i] = boost::python::extract<int>(indexList[i]);

    self.mutableIndexListProperty() = iList;
  }

  static void setIndexType(TypedWorkspacePropertyWithIndex &self,
                           const API::IndexType &indexType) {
    self.mutableIndexTypeProperty() = indexType;
  }

  /**
   * Defines the necessary exports for a
   * WorkspacePropertyWithIndex<WorkspaceType>.
   * to the given class name
   * @param pythonClassName :: The name of the class in python
   */
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using Mantid::API::WorkspaceProperty;
    using Mantid::API::IWorkspacePropertyWithIndex;

    register_ptr_to_python<TypedWorkspacePropertyWithIndex *>();

    class_<TypedWorkspacePropertyWithIndex,
           bases<IWorkspacePropertyWithIndex, WorkspaceProperty<WorkspaceType>>,
           boost::noncopyable>(pythonClassName, no_init)
        .def(init<const std::string &>(args("name")))
        .def(init<const std::string &, const unsigned int>(
            args("name", "indexTypes")))
        .def(init<const std::string &, const unsigned int, const std::string &>(
            args("name", "indexTypes", "defaultValue")))
        .def(init<const std::string &, const unsigned int, const std::string &,
                  API::PropertyMode::Type>(
            args("name", "indexTypes", "defaultValue", "optional")))
        .def(init<const std::string &, const unsigned int, const std::string &,
                  API::PropertyMode::Type, API::LockMode::Type>(
            args("name", "indexTypes", "defaultValue", "optional", "locking")))
        // These variants require the validator object to be cloned
        .def("__init__",
             make_constructor(&createPropertyWithName, default_call_policies(),
                              (arg("name"))))
        .def("__init__", make_constructor(&createPropertyWithIndexType,
                                          default_call_policies(),
                                          (arg("name"), arg("indexType"))))
        .def("__init__",
             make_constructor(&createPropertyWithValidator,
                              default_call_policies(),
                              (arg("name"), arg("indexType"),
                               arg("defaultValue"), arg("validator"))))
        .def("__init__",
             make_constructor(&createPropertyWithOptionalFlag,
                              default_call_policies(),
                              args("name", "indexType", "defaultValue",
                                   "optional", "validator")))
        .def("__init__",
             make_constructor(&createPropertyWithLockFlag,
                              default_call_policies(),
                              args("name", "indexType", "defaultValue",
                                   "optional", "locking", "validator")))
        .def("isOptional", &TypedWorkspacePropertyWithIndex::isOptional,
             arg("self"),
             "Returns true if the property has been marked as optional")
        .def("setIndexType", &setIndexType, (arg("self"), arg("indexType")))
        .def("setIndexList", &setIndexListString,
             (arg("self"), arg("indexList")))
        .def("setIndexList", &setIndexListVector,
             (arg("self"), arg("indexList")))
        .add_property("value", &value)
        .add_property("valueWithIndex", &valueWithIndex);
  }
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYWITHINDEXMACRO_H_ */
