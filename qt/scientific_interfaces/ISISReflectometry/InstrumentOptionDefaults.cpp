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
         lhs.LambdaMin == rhs.LambdaMin && lhs.LambdaMax == rhs.LambdaMax &&
         lhs.I0MonitorIndex == rhs.I0MonitorIndex &&
         lhs.ProcessingInstructions == rhs.ProcessingInstructions &&
         lhs.DetectorCorrectionType == rhs.DetectorCorrectionType;
}

std::ostream &operator<<(std::ostream &os,
                         InstrumentOptionDefaults const &defaults) {
  os << "{ NormalizeByIntegratedMonitors: "
     << (defaults.NormalizeByIntegratedMonitors ? "true" : "false")
     << ",\n MonitorIntegralMin: " << defaults.MonitorIntegralMin
     << ",\n MonitorIntegralMax: " << defaults.MonitorIntegralMax
     << ",\n MonitorBackgroundMin: " << defaults.MonitorBackgroundMin
     << ",\n MonitorBackgroundMax: " << defaults.MonitorBackgroundMax
     << ",\n LambdaMin: " << defaults.LambdaMin
     << ",\n LambdaMax: " << defaults.LambdaMax
     << ",\n I0MonitorIndex: " << defaults.I0MonitorIndex;
  if (defaults.ProcessingInstructions)
    os << ",\n ProcessingInstructions: '" << defaults.ProcessingInstructions.value();
  os << "',\n DetectorCorrectionType: '" << defaults.DetectorCorrectionType
     << "' }" << std::endl;
  return os;
}
}
}
