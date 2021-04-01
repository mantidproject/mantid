// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumConstantBackground

    The function inherits from FlatBackground and also implements the
    IPoldiFunction1D interface.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 07/01/2015
  */
class MANTID_SINQ_DLL PoldiSpectrumConstantBackground : public API::ParamFunction,
                                                        public API::IFunction1D,
                                                        public IPoldiFunction1D {
public:
  PoldiSpectrumConstantBackground();
  std::string name() const override { return "PoldiSpectrumConstantBackground"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  void setWorkspace(std::shared_ptr<const API::Workspace> ws) override;
  size_t getTimeBinCount() const;

  void poldiFunction1D(const std::vector<int> &indices, const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override;
  void setParameter(size_t i, const double &value, bool explicitlySet = true) override;

  double getParameter(const std::string &name) const override;
  double getParameter(size_t i) const override;

protected:
  void init() override;

  size_t m_timeBinCount;
  API::IFunction1D_sptr m_flatBackground;
};

} // namespace Poldi
} // namespace Mantid
