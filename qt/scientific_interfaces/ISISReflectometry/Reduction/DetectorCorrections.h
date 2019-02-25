// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
#include "Common/DllConfig.h"
#include <stdexcept>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class DetectorCorrectionType

    The DetectorCorrectionType determines how detectors should be
    moved prior to redcution
*/
enum class DetectorCorrectionType { VerticalShift, RotateAroundSample };

inline std::string
detectorCorrectionTypeToString(DetectorCorrectionType correctionType) {
  switch (correctionType) {
  case DetectorCorrectionType::VerticalShift:
    return "VerticalShift";
  case DetectorCorrectionType::RotateAroundSample:
    return "RotateAroundSample";
  }
  throw std::runtime_error("Unexpected detector correction type");
}

inline DetectorCorrectionType
detectorCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "VerticalShift")
    return DetectorCorrectionType::VerticalShift;
  if (correctionType == "RotateAroundSample")
    return DetectorCorrectionType::RotateAroundSample;
  throw std::runtime_error("Unexpected detector correction type");
}

/** The DetectorCorrections model holds information about whether
 * and how detectors in a workspace should be moved before being
 * reduced.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL DetectorCorrections {
public:
  DetectorCorrections(bool correctPositions,
                      DetectorCorrectionType correctionType);
  bool correctPositions() const;
  DetectorCorrectionType correctionType() const;

private:
  bool m_correctPositions;
  DetectorCorrectionType m_correctionType;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
