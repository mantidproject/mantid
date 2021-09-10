// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
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
// Forward declarations
//----------------------------------------------------------------------
class IConstraint;
class IFunction;
class Expression;

/** @class Mantid::API::ConstraintFactoryImpl

    The ConstraintFactory class is in charge of the creation of concrete
    instances of Constraints. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 4/02/2010
*/

class MANTID_API_DLL ConstraintFactoryImpl final : public Kernel::DynamicFactory<IConstraint> {
public:
  ConstraintFactoryImpl(const ConstraintFactoryImpl &) = delete;
  ConstraintFactoryImpl &operator=(const ConstraintFactoryImpl &) = delete;
  /// Creates an instance of a Constraint
  IConstraint *createInitialized(IFunction *fun, const std::string &input, bool isDefault = false) const;
  /// Creates an instance of a Constraint
  IConstraint *createInitialized(IFunction *fun, const Expression &expr, bool isDefault = false) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ConstraintFactoryImpl>;

  /// Private Constructor for singleton class
  ConstraintFactoryImpl();
  /// Private Destructor
  ~ConstraintFactoryImpl() override = default;
};

using ConstraintFactory = Mantid::Kernel::SingletonHolder<ConstraintFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::ConstraintFactoryImpl>;
}
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the
 * FunctionFactory
 */
#define DECLARE_CONSTRAINT(classname)                                                                                  \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_constraint_##classname(                                                  \
      ((Mantid::API::ConstraintFactory::Instance().subscribe<classname>(#classname)), 0));                             \
  }
