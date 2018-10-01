#include "PolarizationCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

PolarizationCorrections::PolarizationCorrections(double CRho, double CAlpha,
                                                 double CAp, double CPp)
    : CRho(CRho), CAlpha(CAlpha), CAp(CAp), CPp(CPp) {}

double PolarizationCorrections::cRho() const { return CRho; }

double PolarizationCorrections::cAlpha() const { return CAlpha; }

double PolarizationCorrections::cAp() const { return CAp; }

double PolarizationCorrections::cPp() const { return CPp; }

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
