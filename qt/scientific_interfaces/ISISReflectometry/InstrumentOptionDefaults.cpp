#include "InstrumentOptionDefaults.h"
#include "ValueOr.h"

namespace MantidQt {
namespace CustomInterfaces {
bool operator==(InstrumentOptionDefaults const &lhs,
                InstrumentOptionDefaults const &rhs) {
  return lhs.NormalizeByIntegratedMonitors ==
             rhs.NormalizeByIntegratedMonitors &&
         lhs.MonitorIntegralMin == rhs.MonitorIntegralMin &&
         lhs.MonitorIntegralMax == rhs.MonitorIntegralMax &&
         lhs.MonitorBackgroundMin == rhs.MonitorBackgroundMin &&
         lhs.LambdaMin == rhs.LambdaMin && lhs.LambdaMax == rhs.LambdaMax &&
         lhs.I0MonitorIndex == rhs.I0MonitorIndex &&
         lhs.DetectorCorrectionType == rhs.DetectorCorrectionType;
}

std::ostream &operator<<(std::ostream &os,
                         InstrumentOptionDefaults const &defaults) {
  os << "InstrumentOptionDefaults: { NormalizeByIntegratedMonitors: "
     << (defaults.NormalizeByIntegratedMonitors ? "true" : "false")
     << ",\n MonitorIntegralMin: " << defaults.MonitorIntegralMin
     << ",\n MonitorIntegralMax: " << defaults.MonitorIntegralMax
     << ",\n MonitorBackgroundMin: " << defaults.MonitorBackgroundMin
     << ",\n MonitorBackgroundMax: " << defaults.MonitorBackgroundMax
     << ",\n LambdaMin: " << defaults.LambdaMin
     << ",\n LambdaMax: " << defaults.LambdaMax
     << ",\n I0MonitorIndex: " << defaults.I0MonitorIndex
     << ",\n CorrectDetectors: "
     << (defaults.CorrectDetectors ? "true" : "false");
  os << ",\n DetectorCorrectionType: '" << defaults.DetectorCorrectionType
     << "' }" << std::endl;
  return os;
}
}
}
