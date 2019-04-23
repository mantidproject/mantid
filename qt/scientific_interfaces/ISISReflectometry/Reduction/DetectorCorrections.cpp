// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "DetectorCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

DetectorCorrections::DetectorCorrections(bool correctPositions,
                                         DetectorCorrectionType correctionType)
    : m_correctPositions(correctPositions), m_correctionType(correctionType) {}

bool DetectorCorrections::correctPositions() const {
  return m_correctPositions;
}

DetectorCorrectionType DetectorCorrections::correctionType() const {
  return m_correctionType;
}

bool operator!=(DetectorCorrections const &lhs,
                DetectorCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(DetectorCorrections const &lhs,
                DetectorCorrections const &rhs) {
  return lhs.correctPositions() == rhs.correctPositions() &&
         lhs.correctionType() == rhs.correctionType();
}
} // namespace CustomInterfaces
} // namespace MantidQt
