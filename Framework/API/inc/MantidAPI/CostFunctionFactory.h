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
class ICostFunction;

/** @class CostFunctionFactoryImpl

    The CostFunctionFactory class is in charge of the creation of concrete
    instances of minimizers. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Anders Markvardsen, ISIS, RAL
    @date 20/05/2010
*/

class MANTID_API_DLL CostFunctionFactoryImpl : public Kernel::DynamicFactory<ICostFunction> {
public:
  /**Creates an instance of a cost function
   * @param type :: The function's type
   * @return A pointer to the created function
   */
  ICostFunction *createFunction(const std::string &type) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<CostFunctionFactoryImpl>;
  /// Private Constructor for singleton class
  CostFunctionFactoryImpl();
};

using CostFunctionFactory = Mantid::Kernel::SingletonHolder<CostFunctionFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::CostFunctionFactoryImpl>;
}
} // namespace Mantid
