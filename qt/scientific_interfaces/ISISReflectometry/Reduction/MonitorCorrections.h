#ifndef MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
#include "../DllConfig.h"
#include "RangeInLambda.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL MonitorCorrections {
public:
  MonitorCorrections(size_t monitorIndex, bool integrate,
                     RangeInLambda backgroundRange,
                     RangeInLambda integralRange);

  size_t monitorIndex() const;
  bool integrate() const;
  RangeInLambda backgroundRange() const;
  RangeInLambda integralRange() const;

private:
  size_t m_monitorIndex;
  bool m_integrate;
  RangeInLambda m_backgroundRange;
  RangeInLambda m_integralRange;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
