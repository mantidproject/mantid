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
enum class FloodCorrectionType { None, Workspace, ParameterFile };

inline FloodCorrectionType floodCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "None")
    return FloodCorrectionType::None;
  if (correctionType == "Workspace" || correctionType == "FilePath")
    return FloodCorrectionType::Workspace;
  if (correctionType == "ParameterFile")
    return FloodCorrectionType::ParameterFile;
  else
    throw std::invalid_argument("Unexpected flood correction type.");
}

inline std::string floodCorrectionTypeToString(FloodCorrectionType correctionType) {
  switch (correctionType) {
  case FloodCorrectionType::None:
    return "None";
  case FloodCorrectionType::Workspace:
    return "Workspace";
  case FloodCorrectionType::ParameterFile:
    return "ParameterFile";
  }
  throw std::invalid_argument("Unexpected flood correction type.");
}

inline bool floodCorrectionRequiresInputs(FloodCorrectionType correctionType) {
  return (correctionType == FloodCorrectionType::Workspace);
}

/** @class FloodCorrections

    The FloodCorrections model holds information about what type of flood
    corrections should be performed in the reduction
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL FloodCorrections {
public:
  FloodCorrections(FloodCorrectionType correctionType, std::optional<std::string> workspace = std::nullopt);

  FloodCorrectionType correctionType() const;
  std::optional<std::string> workspace() const;

private:
  FloodCorrectionType m_correctionType;
  std::optional<std::string> m_workspace;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(FloodCorrections const &lhs, FloodCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(FloodCorrections const &lhs, FloodCorrections const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
