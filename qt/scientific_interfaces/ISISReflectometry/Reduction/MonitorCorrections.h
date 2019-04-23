// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
#include "Common/DllConfig.h"
#include "RangeInLambda.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

/** @class MonitorCorrections

    The MonitorCorrections model holds information about which monitor
    normalisations should be applied in reduction
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL MonitorCorrections {
public:
  MonitorCorrections(size_t monitorIndex, bool integrate,
                     boost::optional<RangeInLambda> backgroundRange,
                     boost::optional<RangeInLambda> integralRange);

  size_t monitorIndex() const;
  bool integrate() const;
  boost::optional<RangeInLambda> backgroundRange() const;
  boost::optional<RangeInLambda> integralRange() const;

private:
  size_t m_monitorIndex;
  bool m_integrate;
  boost::optional<RangeInLambda> m_backgroundRange;
  boost::optional<RangeInLambda> m_integralRange;
};

bool operator!=(MonitorCorrections const &lhs, MonitorCorrections const &rhs);
bool operator==(MonitorCorrections const &lhs, MonitorCorrections const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_MONITORCORRECTIONS_H_
