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
#include "ReductionType.h"
#include "SummationType.h"
#include "PolarizationCorrections.h"
#include "RangeInLambda.h"
#include "RowTemplate.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Experiment {
public:
  Experiment(AnalysisMode analysisMode, ReductionType reductionType,
             SummationType summationType,
             PolarizationCorrections polarizationCorrections,
             RangeInLambda transmissionRunRange,
             std::string stitchParameters,
             std::vector<RowTemplate> rowTemplate);

  AnalysisMode analysisMode() const;
  ReductionType reductionType() const;
  SummationType summationType() const;
  PolarizationCorrections const &polarizationCorrections() const;
  RangeInLambda const &transissionRunRange() const;
  std::string stitchParameters() const;
  std::vector<RowTemplate> const &rowTemplates() const;

  RowTemplate const *rowTemplateForTheta(double thetaAngle, double tolerance) const;

private:
  AnalysisMode m_analysisMode;
  ReductionType m_reductionType;
  SummationType m_summationType;

  PolarizationCorrections m_polarizationCorrections;
  RangeInLambda m_transmissionRunRange;

  std::string m_stitchParameters;
  std::vector<RowTemplate> m_rowTemplates;
};
}
}
