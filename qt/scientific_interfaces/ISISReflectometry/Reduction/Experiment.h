// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENT_H_

#include "AnalysisMode.h"
#include "Common/DllConfig.h"
#include "FloodCorrections.h"
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

/** @class Experiment

    The Experiment model holds all settings relating to the Experiment Settings
    tab on the GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Experiment {
public:
  Experiment(AnalysisMode analysisMode, ReductionType reductionType,
             SummationType summationType, bool includePartialBins, bool debug,
             PolarizationCorrections polarizationCorrections,
             FloodCorrections floodCorrections,
             boost::optional<RangeInLambda> transmissionRunRange,
             std::map<std::string, std::string> stitchParameters,
             std::vector<PerThetaDefaults> perThetaDefaults);

  AnalysisMode analysisMode() const;
  ReductionType reductionType() const;
  SummationType summationType() const;
  bool includePartialBins() const;
  bool debug() const;
  PolarizationCorrections const &polarizationCorrections() const;
  FloodCorrections const &floodCorrections() const;
  boost::optional<RangeInLambda> transmissionRunRange() const;
  std::map<std::string, std::string> stitchParameters() const;
  std::string stitchParametersString() const;
  std::vector<PerThetaDefaults> const &perThetaDefaults() const;
  std::vector<std::array<std::string, 8>> perThetaDefaultsArray() const;

  PerThetaDefaults const *defaultsForTheta(double thetaAngle,
                                           double tolerance) const;
  PerThetaDefaults const *wildcardDefaults() const;

private:
  AnalysisMode m_analysisMode;
  ReductionType m_reductionType;
  SummationType m_summationType;
  bool m_includePartialBins;
  bool m_debug;

  PolarizationCorrections m_polarizationCorrections;
  FloodCorrections m_floodCorrections;
  boost::optional<RangeInLambda> m_transmissionRunRange;

  std::map<std::string, std::string> m_stitchParameters;
  std::vector<PerThetaDefaults> m_perThetaDefaults;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(Experiment const &lhs,
                                               Experiment const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(Experiment const &lhs,
                                               Experiment const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
