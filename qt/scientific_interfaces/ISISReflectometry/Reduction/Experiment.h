// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENT_H_

#include "../DllConfig.h"
#include "AnalysisMode.h"
#include "PerThetaDefaults.h"
#include "PolarizationCorrections.h"
#include "RangeInLambda.h"
#include "ReductionType.h"
#include "SummationType.h"
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Experiment {
public:
  Experiment(AnalysisMode analysisMode, ReductionType reductionType,
             SummationType summationType,
             PolarizationCorrections polarizationCorrections,
             boost::optional<RangeInLambda> transmissionRunRange,
             std::map<std::string, std::string> stitchParameters,
             std::vector<PerThetaDefaults> perThetaDefaults);

  AnalysisMode analysisMode() const;
  ReductionType reductionType() const;
  SummationType summationType() const;
  PolarizationCorrections const &polarizationCorrections() const;
  boost::optional<RangeInLambda> transmissionRunRange() const;
  std::map<std::string, std::string> stitchParameters() const;
  std::vector<PerThetaDefaults> const &perThetaDefaults() const;

  PerThetaDefaults const *defaultsForTheta(double thetaAngle,
                                           double tolerance) const;

private:
  AnalysisMode m_analysisMode;
  ReductionType m_reductionType;
  SummationType m_summationType;

  PolarizationCorrections m_polarizationCorrections;
  boost::optional<RangeInLambda> m_transmissionRunRange;

  std::map<std::string, std::string> m_stitchParameters;
  std::vector<PerThetaDefaults> m_perThetaDefaults;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
