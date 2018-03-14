#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#include <string>
#include "MantidGeometry/Instrument.h"
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <ostream>
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentOptionDefaults {
  bool NormalizeByIntegratedMonitors;
  double MonitorIntegralMin;
  double MonitorIntegralMax;
  double MonitorBackgroundMin;
  double MonitorBackgroundMax;
  double LambdaMin;
  double LambdaMax;
  boost::variant<int, double> I0MonitorIndex;
  bool CorrectDetectors;
  std::string DetectorCorrectionType;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(InstrumentOptionDefaults const &lhs,
           InstrumentOptionDefaults const &rhs);

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, InstrumentOptionDefaults const &defaults);
}
}
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
