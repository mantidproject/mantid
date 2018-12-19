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
    PolarizationCorrectionType correctionType, boost::optional<double> CRho,
    boost::optional<double> CAlpha, boost::optional<double> CAp,
    boost::optional<double> CPp)
    : m_correctionType(correctionType), m_cRho(CRho), m_cAlpha(CAlpha),
      m_cAp(CAp), m_cPp(CPp) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() const {
  return m_correctionType;
}

boost::optional<double> PolarizationCorrections::cRho() const { return m_cRho; }

boost::optional<double> PolarizationCorrections::cAlpha() const {
  return m_cAlpha;
}

boost::optional<double> PolarizationCorrections::cAp() const { return m_cAp; }

boost::optional<double> PolarizationCorrections::cPp() const { return m_cPp; }

bool operator!=(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() &&
         lhs.cRho() == rhs.cRho() && lhs.cAlpha() == rhs.cAlpha() &&
         lhs.cAp() == rhs.cAp() && lhs.cPp() == rhs.cPp();
}
} // namespace CustomInterfaces
} // namespace MantidQt
