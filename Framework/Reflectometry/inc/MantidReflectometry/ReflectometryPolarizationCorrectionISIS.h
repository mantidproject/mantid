// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidReflectometry/DllConfig.h"

#include <functional>
#include <string>

namespace Mantid::Reflectometry {

class MANTID_REFLECTOMETRY_DLL ReflectometryPolarizationCorrectionISIS {
public:
  using ChildAlgorithmFactory = std::function<API::Algorithm_sptr(const std::string &)>;
  using IsDefault = std::function<bool(const std::string &)>;
  using StringPropertyGetter = std::function<std::string(const std::string &)>;
  using MatrixWorkspacePropertyGetter = std::function<API::MatrixWorkspace_sptr(const std::string &)>;

  ReflectometryPolarizationCorrectionISIS(const API::WorkspaceGroup_sptr &inputWorkspaces,
                                          ChildAlgorithmFactory childAlgorithmFactory, IsDefault isDefault,
                                          StringPropertyGetter getPropertyValue,
                                          MatrixWorkspacePropertyGetter getMatrixWorkspaceProperty);

  API::WorkspaceGroup_sptr apply(const std::string &outputGroupName) const;

private:
  struct Correction {
    API::MatrixWorkspace_sptr efficiencies;
    std::string method;
    std::string option;
    std::string fredrikzeInputSpinStateOrder;
  };

  std::string findCorrectionMethod(const API::MatrixWorkspace_sptr &efficiencies) const;
  std::string findCorrectionOption(const std::string &correctionMethod) const;
  std::string findFredrikzeInputSpinStateOrder(const std::string &correctionMethod,
                                               const std::string &correctionOption) const;
  std::string getCustomInputSpinStateOrder() const;
  void validateInputSpinStateOrderFamily(const std::string &correctionMethod, const std::string &spinStates) const;
  Correction getCorrection() const;

  API::WorkspaceGroup_sptr m_inputWorkspaces;
  ChildAlgorithmFactory m_childAlgorithmFactory;
  IsDefault m_isDefault;
  StringPropertyGetter m_getPropertyValue;
  MatrixWorkspacePropertyGetter m_getMatrixWorkspaceProperty;
};

} // namespace Mantid::Reflectometry
