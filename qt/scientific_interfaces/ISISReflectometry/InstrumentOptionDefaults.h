#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
#include <string>
#include "MantidGeometry/Instrument.h"

namespace MantidQt {
namespace CustomInterfaces {

struct InstrumentOptionDefaults {
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

}
}
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTOPTIONDEFAULTS_H
