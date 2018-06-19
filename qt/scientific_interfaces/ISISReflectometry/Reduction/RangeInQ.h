#ifndef MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#define MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInQ {
public:
  RangeInQ(double min, double step, double max);
  double min() const;
  double max() const;
  double step() const;

private:
  double m_min;
  double m_step;
  double m_max;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
}
}
#endif // MANTID_CUSTOMINTERFACES_RANGEINQ_H_
