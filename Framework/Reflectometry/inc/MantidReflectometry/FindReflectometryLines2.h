// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** FindReflectometryLines2 : Finds fractional workspace index
  corresponding to reflected or direct line in a line detector workspace.
*/
class MANTID_REFLECTOMETRY_DLL FindReflectometryLines2 final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  double findPeak(const API::MatrixWorkspace_sptr &ws);
  API::MatrixWorkspace_sptr integrate(const API::MatrixWorkspace_sptr &ws);
  API::MatrixWorkspace_sptr transpose(const API::MatrixWorkspace_sptr &ws);
};

} // namespace Reflectometry
} // namespace Mantid
