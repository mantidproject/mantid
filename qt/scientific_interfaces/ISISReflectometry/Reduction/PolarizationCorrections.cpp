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
}
}
