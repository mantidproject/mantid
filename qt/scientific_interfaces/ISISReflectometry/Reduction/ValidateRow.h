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
#ifndef MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#include "Row.h"
#include <boost/optional.hpp>
#include "Reduction/ReductionJobs.h"
#include "ValidationResult.h"
#include "ParseReflectometryStrings.h"
#include "TransmissionRunPair.h"
#include "DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

template <typename Row> class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidator {
public:
  template <typename WorkspaceNamesFactory>
  ValidationResult<RowVariant, std::vector<int>>
  operator()(std::vector<std::string> const &cellText,
             WorkspaceNamesFactory const &workspaceNames);

private:
  boost::optional<std::vector<std::string>>
  parseRunNumbers(std::vector<std::string> const &cellText);
  boost::optional<double> parseTheta(std::vector<std::string> const &cellText);
  boost::optional<TransmissionRunPair>
  parseTransmissionRuns(std::vector<std::string> const &cellText);
  boost::optional<boost::optional<RangeInQ>>
  parseQRange(std::vector<std::string> const &cellText);
  boost::optional<boost::optional<double>>
  parseScaleFactor(std::vector<std::string> const &cellText);
  boost::optional<std::map<std::string, std::string>>
  parseOptions(std::vector<std::string> const &cellText);

  std::vector<int> m_invalidColumns;
};

template <typename Row>
using RowValidationResult = ValidationResult<Row, std::vector<int>>;

RowValidationResult<RowVariant>
validateRow(Jobs const &jobs,
            WorkspaceNamesFactory const &workspaceNamesFactory,
            std::vector<std::string> const &cellText);

boost::optional<RowVariant>
validateRowFromRunAndTheta(Jobs const &jobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory,
                           std::string const &run, std::string const &theta);
}
}
#endif // MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
