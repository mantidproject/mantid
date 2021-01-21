// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <vector>

namespace Mantid {

namespace API {

//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
class ScriptRepository;

/** @class ScriptRepositoryFactoryImpl

    The ScriptRepositoryFactoryImpl class is in charge of the creation of
   concrete
    instance of ScriptRepository interface. It inherits most of its
   implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Gesner Passos, ISIS
    @date 20/12/2012
*/

class MANTID_API_DLL ScriptRepositoryFactoryImpl : public Kernel::DynamicFactory<ScriptRepository> {
public:
  ScriptRepositoryFactoryImpl(const ScriptRepositoryFactoryImpl &) = delete;
  ScriptRepositoryFactoryImpl &operator=(const ScriptRepositoryFactoryImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ScriptRepositoryFactoryImpl>;

  /// Private Constructor for singleton class
  ScriptRepositoryFactoryImpl();
  /// Private Destructor
  ~ScriptRepositoryFactoryImpl() override = default;
};

using ScriptRepositoryFactory = Mantid::Kernel::SingletonHolder<ScriptRepositoryFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::ScriptRepositoryFactoryImpl>;
}
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the
 * FunctionFactory
 */
#define DECLARE_SCRIPTREPOSITORY(classname)                                                                            \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_function_##classname(                                                    \
      ((Mantid::API::ScriptRepositoryFactory::Instance().subscribe<classname>(#classname)), 0));                       \
  }
