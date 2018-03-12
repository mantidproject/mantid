#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
#include <string>
#include <ostream>
#include "DllConfig.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

struct MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentOptionDefaults {
  std::string AnalysisMode;
  std::string PolarizationAnalysis;
  std::string SummationType;
  std::string ReductionType;
  std::string CRho;
  std::string CAlpha;
  std::string CAp;
  std::string CPp;
  boost::optional<double> TransRunStartOverlap;
  boost::optional<double> TransRunEndOverlap;
  boost::optional<double> MomentumTransferMin;
  boost::optional<double> MomentumTransferMax;
  boost::optional<double> MomentumTransferStep;
  boost::optional<double> ScaleFactor;
  boost::optional<std::string> ProcessingInstructions;
  boost::optional<std::string> StitchParams;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(const ExperimentOptionDefaults &lhs,
           const ExperimentOptionDefaults &rhs);

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, ExperimentOptionDefaults const &defaults);
}
}
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTOPTIONDEFAULTS_H
