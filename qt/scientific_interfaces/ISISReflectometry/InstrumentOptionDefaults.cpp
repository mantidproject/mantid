#include "InstrumentOptionDefaults.h"
namespace MantidQt {
namespace CustomInterfaces {
bool operator==(InstrumentOptionDefaults const &lhs,
                InstrumentOptionDefaults const &rhs) {
  return lhs.NormalizeByIntegratedMonitors ==
             rhs.NormalizeByIntegratedMonitors &&
         lhs.MonitorIntegralMin == rhs.MonitorIntegralMin &&
         lhs.MonitorIntegralMax == rhs.MonitorIntegralMax &&
         lhs.MonitorBackgroundMin == rhs.MonitorBackgroundMin &&
         lhs.LambdaMin == rhs.LambdaMax &&
         lhs.I0MonitorIndex == rhs.I0MonitorIndex &&
         lhs.ProcessingInstructions == rhs.ProcessingInstructions &&
         lhs.DetectorCorrectionType == rhs.DetectorCorrectionType;
}

std::ostream &operator<<(std::ostream &os,
                         InstrumentOptionDefaults const &defaults) {
  os << "{ NormalizeByIntegratedMonitors: "
     << (defaults.NormalizeByIntegratedMonitors ? "true" : "false")
     << ", MonitorIntegralMin: " << defaults.MonitorIntegralMin
     << ", MonitorIntegralMax: " << defaults.MonitorIntegralMax
     << ", MonitorBackgroundMin: " << defaults.MonitorBackgroundMin
     << ", MonitorBackgroundMax: " << defaults.MonitorBackgroundMax
     << ", LambdaMin: " << defaults.LambdaMin << ", LambdaMax: '"
     << defaults.LambdaMax << ", I0MonitorIndex: " << defaults.I0MonitorIndex
     << ", ProcessingInstructions: '" << defaults.ProcessingInstructions
     << "', DetectorCorrectionType: '" << defaults.DetectorCorrectionType
     << std::endl;
  return os;
}
}
}
