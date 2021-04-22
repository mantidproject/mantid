// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
enum class PolarizationCorrectionType { None, ParameterFile };

inline PolarizationCorrectionType polarizationCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "None")
    return PolarizationCorrectionType::None;
  else if (correctionType == "ParameterFile")
    return PolarizationCorrectionType::ParameterFile;
  else
    throw std::invalid_argument("Unexpected polarization correction type.");
}

inline std::string polarizationCorrectionTypeToString(PolarizationCorrectionType correctionType) {
  switch (correctionType) {
  case PolarizationCorrectionType::None:
    return "None";
  case PolarizationCorrectionType::ParameterFile:
    return "ParameterFile";
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
  explicit PolarizationCorrections(PolarizationCorrectionType correctionType);

  PolarizationCorrectionType correctionType() const;

private:
  PolarizationCorrectionType m_correctionType;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt