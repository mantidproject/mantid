// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

/** IFunctionGeneral: a very general function definition.
    It gets its arguments from a FunctionDomainGeneral and they
    can have any type. An argument can be a collection of a number
    of values of different types.

    The domain and the values object can have different sizes.
    In particular the domain can be empty.
  */
class MANTID_API_DLL IFunctionGeneral : public virtual IFunction {
public:
  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  size_t getValuesSize(const FunctionDomain &domain) const override;

  /// Provide a concrete function in an implementation that operates on a
  /// FunctionDomainGeneral.
  virtual void functionGeneral(const FunctionDomainGeneral &domain, FunctionValues &values) const = 0;

  /// Get number of columns that the domain must have.
  /// If consider the collection of these columns as a table
  /// then a row corresponds to a single (multi-valued) argument.
  virtual size_t getNumberDomainColumns() const = 0;

  /// Get number of values per argument in the domain.
  virtual size_t getNumberValuesPerArgument() const = 0;

  /// Get the default size of a domain.
  /// If a function is given an empty domain then it must output
  /// a values object of the size:
  ///     getDefaultDomainSize() * getNumberValuesPerArgument()
  /// The default size must not be infinite (max of size_t).
  virtual size_t getDefaultDomainSize() const;

protected:
  static Kernel::Logger g_log;
};

} // namespace API
} // namespace Mantid
