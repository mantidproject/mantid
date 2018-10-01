#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENT_H_
/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
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
