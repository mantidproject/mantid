// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Column.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** ConvolutionFit : Performs a QENS convolution fit
 */
template <typename Base> class MANTID_CURVEFITTING_DLL ConvolutionFit : public Base {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  virtual API::ITableWorkspace_sptr processParameterTable(API::ITableWorkspace_sptr parameterTable) override;
  std::map<std::string, std::string> getAdditionalLogStrings() const override;
  std::map<std::string, std::string> getAdditionalLogNumbers() const override;
  std::vector<std::string> getFitParameterNames() const override;

private:
  std::map<std::string, std::string> validateInputs() override;

  bool throwIfElasticQConversionFails() const override;
  bool isFitParameter(const std::string &name) const override;

  bool m_deltaUsed;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
