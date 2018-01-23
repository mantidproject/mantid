#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#include <string>
#include <ostream>
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
struct MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentOptionDefaults {
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

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(const ExperimentOptionDefaults &lhs,
           const ExperimentOptionDefaults &rhs);

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, ExperimentOptionDefaults const &defaults);
}
}
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
