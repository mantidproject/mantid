// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PolarizationCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

PolarizationCorrections::PolarizationCorrections(
    PolarizationCorrectionType correctionType)
    : m_correctionType(correctionType) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() const {
  return m_correctionType;
}

bool operator!=(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType();
}
} // namespace CustomInterfaces
} // namespace MantidQt
