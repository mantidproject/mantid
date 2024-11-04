// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TableWorkspaceNotEmptyValidator.h"
#include "MantidDataObjects/TableWorkspaceValidator.h"
#include "MantidPythonInterface/core/TypedValidatorExporter.h"
#include <boost/python/class.hpp>

using Mantid::API::ITableWorkspace_sptr;
using Mantid::Kernel::TypedValidator;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;

/// This is the base TypedValidator for most of the WorkspaceValidators
void export_TableWorkspaceValidator() {
  using Mantid::DataObjects::TableWorkspace_sptr;
  using Mantid::DataObjects::TableWorkspaceValidator;
  TypedValidatorExporter<TableWorkspace_sptr>::define("TableWorkspaceValidator");
  TypedValidatorExporter<ITableWorkspace_sptr>::define("ITableWorkspaceValidator");

  class_<TableWorkspaceValidator, bases<TypedValidator<TableWorkspace_sptr>>, boost::noncopyable>(
      "TableWorkspaceValidator", no_init);

  class_<TypedValidator<ITableWorkspace_sptr>, boost::noncopyable>("ITableWorkspaceValidator", no_init);
}
/// Export a validator derived from a TableWorkspaceValidator that has a no-arg
/// constructor
#define EXPORT_WKSP_VALIDATOR_NO_ARG(ValidatorType, DocString)                                                         \
  class_<ValidatorType, bases<TableWorkspaceValidator>, boost::noncopyable>(#ValidatorType, init<>(DocString));
/// Export a validator derived from a MatrixWorkspaceValidator that has a
/// single-arg constructor
#define EXPORT_WKSP_VALIDATOR_ARG(ValidatorType, ArgType, ArgName, DocString)                                          \
  class_<ValidatorType, bases<TableWorkspaceValidator>, boost::noncopyable>(#ValidatorType,                            \
                                                                            init<ArgType>(arg(ArgName), DocString));
/// Export a validator derived from a MatrixWorkspaceValidator that has a
/// single-arg constructor
/// with a default argument
#define EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(ValidatorType, ArgType, ArgName, DefaultValue, DocString)                    \
  class_<ValidatorType, bases<TableWorkspaceValidator>, boost::noncopyable>(                                           \
      #ValidatorType, init<ArgType>(arg(ArgName) = DefaultValue, DocString));

void export_WorkspaceValidators() {
  using namespace Mantid::DataObjects;

  EXPORT_WKSP_VALIDATOR_NO_ARG(TableWorkspaceNotEmptyValidator, "Checks that the workspace is not empty");
}
