#ifndef MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#include "Reduction/Row.h"
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Param>
bool allInitialized(boost::optional<Param> const &param) {
  return param.is_initialized();
}

template <typename FirstParam, typename SecondParam, typename... Params>
bool allInitialized(boost::optional<FirstParam> const &first,
                    boost::optional<SecondParam> const &second,
                    boost::optional<Params> const &... params) {
  return first.is_initialized() && allInitialized(second, params...);
}

template <typename Result, typename... Params>
Result makeUsingValues(boost::optional<Params> const &... params) {
  return Result(params.get()...);
}

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

template <typename Row>
RowValidationResult<Row> validateRow(std::vector<std::string> const &cellText);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    RowValidationResult<SlicedRow>;
extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    RowValidationResult<SingleRow>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SlicedRow>
validateRow(std::vector<std::string> const &);
extern template MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SingleRow>
validateRow(std::vector<std::string> const &);
}
}
#endif // MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
