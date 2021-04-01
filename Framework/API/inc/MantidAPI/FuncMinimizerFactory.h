// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
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
class IFuncMinimizer;

/** @class FuncMinimizerFactoryImpl

    The FuncMinimizerFactory class is in charge of the creation of concrete
    instances of minimizers. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Anders Markvardsen, ISIS, RAL
    @date 20/05/2010
*/

class MANTID_API_DLL FuncMinimizerFactoryImpl : public Kernel::DynamicFactory<IFuncMinimizer> {
public:
  /// Creates an instance of a minimizer
  std::shared_ptr<IFuncMinimizer> createMinimizer(const std::string &str) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<FuncMinimizerFactoryImpl>;
  /// Private Constructor for singleton class
  FuncMinimizerFactoryImpl();
};

using FuncMinimizerFactory = Mantid::Kernel::SingletonHolder<FuncMinimizerFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::FuncMinimizerFactoryImpl>;
}
} // namespace Mantid

/**
 * Macro for declaring a new type of minimizers to be used with the
 * FuncMinimizerFactory
 */
#define DECLARE_FUNCMINIMIZER(classname, username)                                                                     \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_funcminimizer_##classname(                                               \
      ((Mantid::API::FuncMinimizerFactory::Instance().subscribe<classname>(#username)), 0));                           \
  }
