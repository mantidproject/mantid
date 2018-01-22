#include "ExperimentOptionDefaults.h"
namespace MantidQt {
namespace CustomInterfaces {
bool operator==(const ExperimentOptionDefaults &lhs,
                const ExperimentOptionDefaults &rhs) {
  return lhs.AnalysisMode == rhs.AnalysisMode &&
         lhs.PolarizationAnalysis == rhs.PolarizationAnalysis &&
         lhs.CRho == rhs.CRho && lhs.CAlpha == rhs.CAlpha &&
         lhs.CAp == rhs.CAp && lhs.CPp == rhs.CPp &&
         lhs.TransRunStartOverlap == rhs.TransRunStartOverlap &&
         lhs.TransRunEndOverlap == rhs.TransRunEndOverlap &&
         lhs.MomentumTransferStep == rhs.MomentumTransferStep &&
         lhs.ScaleFactor == rhs.ScaleFactor &&
         lhs.StitchParams == rhs.StitchParams;
}

std::ostream &operator<<(std::ostream &os,
                         ExperimentOptionDefaults const &defaults) {
  os << "{ AnalysisMode: '" << defaults.AnalysisMode
     << ", PolarizationAnalysis: '" << defaults.PolarizationAnalysis
     << "', CRho: '" << defaults.CRho << "', CAlpha: '" << defaults.CAlpha
     << "', CAp: '" << defaults.CAp << "', CPp: '" << defaults.CPp
     << "', TransRunStartOverlap: " << defaults.TransRunStartOverlap
     << ", TransRunEndOverlap: " << defaults.TransRunEndOverlap
     << ", MomentumTransferStep: " << defaults.MomentumTransferStep
     << ", ScaleFactor: " << defaults.ScaleFactor << "', StitchParams: '"
     << defaults.StitchParams << " }" << std::endl;
  return os;
}
}
}
