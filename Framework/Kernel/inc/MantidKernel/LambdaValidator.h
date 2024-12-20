// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/WarningSuppressions.h"
#include <functional>

namespace Mantid::Kernel {

/**
LambdaValidator provides a quick way to create custom validation objects
using a validator function or lambda expression. The class uses TypedValidator
to extract the parameter type and then pass it to the vaildtor function.

The function used for validation should accept one parameter, the variable to be validated,
and returns an error string (empty string for no error).
 */
template <typename ParamType> class MANTID_KERNEL_DLL LambdaValidator : public TypedValidator<ParamType> {
  using ValidatorFunction = std::function<std::string(ParamType)>;

public:
  LambdaValidator()
      : m_validatorFunction([](GNU_UNUSED ParamType x) { return "Error: validator function is not initialized"; }) {}
  LambdaValidator(const ValidatorFunction &validatorFunction) : m_validatorFunction(validatorFunction) {};

  IValidator_sptr clone() const override { return std::make_shared<LambdaValidator>(*this); }

  void setValidatorFunction(const ValidatorFunction &validatorFunction) { m_validatorFunction = validatorFunction; };

private:
  std::string checkValidity(const ParamType &value) const override { return m_validatorFunction(value); }

  ValidatorFunction m_validatorFunction;
};

// namespace Kernel

} // namespace Mantid::Kernel
