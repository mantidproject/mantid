#ifndef MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#include "../DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections {
public:
  PolarizationCorrections(double CRho, double CAlpha, double CAp, double CPp);

  double cRho() const;
  double cAlpha() const;
  double cAp() const;
  double cPp() const;

private:
  double CRho;
  double CAlpha;
  double CAp;
  double CPp;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
