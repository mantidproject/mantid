// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <optional>
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
enum class PolarizationCorrectionType { None, ParameterFile, Workspace };

inline PolarizationCorrectionType polarizationCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "None")
    return PolarizationCorrectionType::None;
  if (correctionType == "ParameterFile")
    return PolarizationCorrectionType::ParameterFile;
  if (correctionType == "Workspace" || correctionType == "FilePath")
    return PolarizationCorrectionType::Workspace;

  throw std::invalid_argument("Unexpected polarization correction type.");
}

inline std::string polarizationCorrectionTypeToString(PolarizationCorrectionType correctionType) {
  switch (correctionType) {
  case PolarizationCorrectionType::None:
    return "None";
  case PolarizationCorrectionType::ParameterFile:
    return "ParameterFile";
  case PolarizationCorrectionType::Workspace:
    return "Workspace";
  }
  throw std::invalid_argument("Unexpected polarization correction type.");
}

/** @class PoliarizationCorrections

    The PoliarizationCorrections model holds information about what
    polarization corrections should be done during reduction. Currently there
    are just two options (namely, apply or don't apply corrections) but this
    may be expanded to include more cases in the future.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections {
public:
  explicit PolarizationCorrections(PolarizationCorrectionType correctionType,
                                   std::optional<std::string> workspace = std::nullopt,
                                   std::string const &fredrikzeSpinStateOrder = "");

  PolarizationCorrectionType correctionType() const;
  std::optional<std::string> workspace() const;
  std::string const &fredrikzeSpinStateOrder() const;

private:
  PolarizationCorrectionType m_correctionType;
  std::optional<std::string> m_workspace;
  std::string m_fredrikzeSpinStateOrder;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
