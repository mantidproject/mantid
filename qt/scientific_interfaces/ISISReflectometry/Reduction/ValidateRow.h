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
#include "Reduction/Row.h"
#include "Reduction/ReductionJobs.h"
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseDouble(std::string string);
MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseNonNegativeDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseNonNegativeNonZeroDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int>
parseInt(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int>
parseNonNegativeInt(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::vector<std::string>>
parseRunNumbers(std::string const &runNumbers);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::string>
parseRunNumber(std::string const &runNumberString);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::string>
parseRunNumberOrWhitespace(std::string const &runNumberString);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseTheta(std::string const &theta);

bool isEntirelyWhitespace(std::string const &string);

using TransmissionRunPair = std::pair<std::string, std::string>;

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::variant<TransmissionRunPair, std::vector<int>>
parseTransmissionRuns(std::string const &firstTransmissionRun,
                      std::string const &secondTransmissionRun);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::variant<boost::optional<RangeInQ>, std::vector<int>>
parseQRange(std::string const &min, std::string const &max,
            std::string const &step);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<boost::optional<double>>
parseScaleFactor(std::string const &scaleFactor);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<std::map<std::string, std::string>>
parseOptions(std::string const &options);

template <typename Row> class RowValidationResult {
public:
  explicit RowValidationResult(Row row);
  explicit RowValidationResult(std::vector<int> invalidColumns);

  bool isValid() const;
  std::vector<int> const &invalidColumns() const;
  boost::optional<Row> const &validRowElseNone() const;

private:
  std::vector<int> m_invalidColumns;
  boost::optional<Row> m_validRow;
};

template <typename Row> class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidator {
public:
  template <typename WorkspaceNamesFactory>
  RowValidationResult<boost::variant<SlicedRow, UnslicedRow>>
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

RowValidationResult<RowVariant>
validateRow(Jobs const &jobs,
            WorkspaceNamesFactory const &workspaceNamesFactory,
            std::vector<std::string> const &cellText);

boost::optional<RowVariant>
validateRowFromRunAndTheta(Jobs const &jobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory,
                           std::string const &run, std::string const &theta);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    RowValidationResult<SlicedRow>;
extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    RowValidationResult<UnslicedRow>;
extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    RowValidationResult<RowVariant>;
}
}
#endif // MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
