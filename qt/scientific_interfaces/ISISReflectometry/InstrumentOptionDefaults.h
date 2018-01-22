#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#include <string>
#include "MantidGeometry/Instrument.h"
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
  int I0MonitorIndex;
  std::string ProcessingInstructions;
  std::string DetectorCorrectionType;
};

bool MANTIDQT_ISISREFLECTOMETRY_DLL
operator==(InstrumentOptionDefaults const &lhs,
           InstrumentOptionDefaults const &rhs);

std::ostream MANTIDQT_ISISREFLECTOMETRY_DLL &
operator<<(std::ostream &os, InstrumentOptionDefaults const &defaults);
}
}
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
