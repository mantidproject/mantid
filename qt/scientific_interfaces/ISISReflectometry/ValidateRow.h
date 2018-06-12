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
  RowValidationResult(Row row);
  RowValidationResult(std::vector<int> invalidColumns);

  bool isValid() const;
  std::vector<int> const &invalidColumns() const;
  boost::optional<Row> const &validRowElseNone() const;

private:
  std::vector<int> m_invalidColumns;
  boost::optional<Row> m_validRow;
};

template <typename Row> class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidator {
public:
  template <typename WorkspaceNameFactory>
  RowValidationResult<boost::variant<SlicedRow, UnslicedRow>>
  operator()(std::vector<std::string> const &cellText,
             WorkspaceNameFactory workspaceNames);

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

RowValidationResult<boost::variant<SlicedRow, UnslicedRow>>
validateRow(Jobs const &jobs, Slicing const &slicing,
            std::vector<std::string> const &cellText);

boost::optional<boost::variant<SlicedRow, UnslicedRow>>
validateRowFromRunAndTheta(Jobs const &jobs, Slicing const &slicing,
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
