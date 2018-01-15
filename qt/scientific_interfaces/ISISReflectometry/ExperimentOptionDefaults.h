#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#include <string>
#include "MantidGeometry/Instrument.h"

namespace MantidQt {
namespace CustomInterfaces {
struct ExperimentOptionDefaults {
  std::string AnalysisMode;
  std::string PolarizationAnalysis;
  std::string CRho;
  std::string CAlpha;
  std::string CAp;
  std::string CPp;
  double TransRunStartOverlap;
  double TransRunEndOverlap;
  double MomentumTransferStep;
  double ScaleFactor;
  std::string StitchParams;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
