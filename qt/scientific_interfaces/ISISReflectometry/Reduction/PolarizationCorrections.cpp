#include "PolarizationCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

PolarizationCorrections::PolarizationCorrections(
    PolarizationCorrectionType correctionType, double CRho, double CAlpha,
    double CAp, double CPp)
    : m_correctionType(correctionType), m_cRho(CRho), m_cAlpha(CAlpha),
      m_cAp(CAp), m_cPp(CPp) {}

PolarizationCorrectionType PolarizationCorrections::correctionType() {
  return m_correctionType;
}

double PolarizationCorrections::cRho() const { return m_cRho; }

double PolarizationCorrections::cAlpha() const { return m_cAlpha; }

double PolarizationCorrections::cAp() const { return m_cAp; }

double PolarizationCorrections::cPp() const { return m_cPp; }

bool operator!=(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return !(lhs == rhs);
}

bool operator==(PolarizationCorrections const &lhs,
                PolarizationCorrections const &rhs) {
  return lhs.cRho() == rhs.cRho() && lhs.cAlpha() == rhs.cAlpha() &&
         lhs.cAp() == rhs.cAp() && lhs.cPp() == rhs.cPp();
}
} // namespace CustomInterfaces
} // namespace MantidQt
