// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/WeakPtr.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

#include <boost/python/register_ptr_to_python.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * Encapsulates the registration required for an interface type T
 * that sits on top of a Kernel::DataItem object. The constructor
 * does 3 things:
 *    - Calls register_ptr_to_python<std::shared_ptr<T>>
 *    - Calls register_ptr_to_python<std::weak_ptr<T>>
 *    - Registers a new PropertyValueHandler for a std::shared_ptr<T>
 */
template <typename IType> struct DLLExport RegisterWorkspacePtrToPython {
  using IType_sptr = std::shared_ptr<IType>;
  using IType_wptr = std::weak_ptr<IType>;
  /// Constructor
  RegisterWorkspacePtrToPython() {
    using namespace boost::python;
    using namespace Registry;

    register_ptr_to_python<IType_sptr>();
    register_ptr_to_python<IType_wptr>();
    // properties can only ever store pointers to these
    TypeRegistry::subscribe<TypedPropertyValueHandler<IType_sptr>>();
  }
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
