#include "ExperimentOptionDefaults.h"
#include "ValueOr.h"

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
         lhs.ReductionType == rhs.ReductionType &&
         lhs.SummationType == rhs.SummationType &&
         lhs.StitchParams == rhs.StitchParams;
}

std::ostream &operator<<(std::ostream &os,
                         ExperimentOptionDefaults const &defaults) {
  os << "ExperimentOptionDefaults: { AnalysisMode: '" << defaults.AnalysisMode
     << ", \nPolarizationAnalysis: '" << defaults.PolarizationAnalysis
     << "',\nCRho: '" << defaults.CRho << "',\nCAlpha: '" << defaults.CAlpha
     << "',\nCAp: '" << defaults.CAp << "', \nCPp: '" << defaults.CPp
     << "',\nSummationType: '" << defaults.SummationType
     << "', \nReductionType: '" << defaults.ReductionType;
  if (defaults.TransRunStartOverlap)
    os << "',\nTransRunStartOverlap: " << defaults.TransRunStartOverlap.get();
  if (defaults.TransRunEndOverlap)
    os << ",\nTransRunEndOverlap: " << defaults.TransRunEndOverlap.get();
  if (defaults.MomentumTransferStep)
    os << ",\nMomentumTransferStep: " << defaults.MomentumTransferStep.get();
  if (defaults.ScaleFactor)
    os << ",\nScaleFactor: " << defaults.ScaleFactor.get();
  if (defaults.StitchParams)
    os << ", \nStitchParams: '" << defaults.StitchParams.get();
  os << "' }" << std::endl;
  return os;
}
}
}
