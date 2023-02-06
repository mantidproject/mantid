// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/TypedValidator.h"
#include <functional>

namespace Mantid::Kernel {

template <typename ParamType> class DLLExport LambdaValidator : public TypedValidator<ParamType> {
  using ValidatorFunction = std::function<std::string(ParamType)>;

public:
  LambdaValidator(const ValidatorFunction &validatorFunction) : m_validatorFunction(validatorFunction){};

  IValidator_sptr clone() const override { return std::make_shared<LambdaValidator>(*this); }

  void setValidatorFunction(const ValidatorFunction &validatorFunction) { m_validatorFunction = validatorFunction; };

private:
  std::string checkValidity(const ParamType &value) const override { return m_validatorFunction(value); }

  ValidatorFunction m_validatorFunction;
};

// namespace Kernel

} // namespace Mantid::Kernel