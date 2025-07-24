// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/EmptyValues.h"

namespace Mantid {
namespace Algorithms {

/** DetermineSpinStateOrder : Takes a workspace group of Polarised SANS run periods and returns
a string (e.g '11, 10, 01, 00') of their corresponding flipper/analyser states.
 */
class MANTID_ALGORITHMS_DLL DetermineSpinStateOrder : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

protected:
  double averageTransmission(Mantid::API::WorkspaceGroup_const_sptr const &wsGroup) const;

private:
  void init() override;
  void exec() override;

  std::string m_spinFlipperLogName = "";
  double m_rfStateCondition = EMPTY_DBL();
};

} // namespace Algorithms
} // namespace Mantid
