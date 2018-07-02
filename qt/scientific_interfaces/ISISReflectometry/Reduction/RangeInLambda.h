#ifndef MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
#define MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
#include "../DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda {
public:
  RangeInLambda(double min, double max);
  double min() const;
  double max() const;

private:
  double m_min, m_max;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
