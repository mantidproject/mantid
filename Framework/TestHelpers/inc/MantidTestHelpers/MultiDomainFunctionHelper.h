// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {

namespace TestHelpers {

class MultiDomainFunctionTest_Function : public Mantid::API::IFunction1D, public Mantid::API::ParamFunction {
public:
  MultiDomainFunctionTest_Function();

  std::string name() const override { return "MultiDomainFunctionTest_Function"; }

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override;
};

std::shared_ptr<Mantid::API::MultiDomainFunction> makeMultiDomainFunction3();

std::shared_ptr<Mantid::API::JointDomain> makeMultiDomainDomain3();

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace1();

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace2();

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace3();

} // namespace TestHelpers
} // namespace Mantid
