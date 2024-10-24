// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <stdexcept>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class DetectorCorrectionType

    The DetectorCorrectionType determines how detectors should be
    moved prior to redcution
*/
enum class DetectorCorrectionType { VerticalShift, RotateAroundSample };

inline std::string detectorCorrectionTypeToString(DetectorCorrectionType correctionType) {
  switch (correctionType) {
  case DetectorCorrectionType::VerticalShift:
    return "VerticalShift";
  case DetectorCorrectionType::RotateAroundSample:
    return "RotateAroundSample";
  }
  throw std::invalid_argument("Unexpected detector correction type");
}

inline DetectorCorrectionType detectorCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "VerticalShift")
    return DetectorCorrectionType::VerticalShift;
  if (correctionType == "RotateAroundSample")
    return DetectorCorrectionType::RotateAroundSample;
  throw std::invalid_argument("Unexpected detector correction type");
}

/** The DetectorCorrections model holds information about whether
 * and how detectors in a workspace should be moved before being
 * reduced.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL DetectorCorrections {
public:
  DetectorCorrections(bool correctPositions, DetectorCorrectionType correctionType);
  bool correctPositions() const;
  DetectorCorrectionType correctionType() const;

private:
  bool m_correctPositions;
  DetectorCorrectionType m_correctionType;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(DetectorCorrections const &lhs, DetectorCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(DetectorCorrections const &lhs, DetectorCorrections const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
