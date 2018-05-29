#include "ValidateRow.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row>
RowValidationResult<Row>::RowValidationResult(Row row)
    : m_invalidColumns(), m_validRow(std::move(row)) {}

template <typename Row>
RowValidationResult<Row>::RowValidationResult(std::vector<int> invalidColumns)
    : m_invalidColumns(std::move(invalidColumns)), m_validRow(boost::none) {}

template <typename Row>
bool RowValidationResult<Row>::isValid() const {
  return m_validRow.is_initialized();
}

template <typename Row>
std::vector<int> const &RowValidationResult<Row>::invalidColumns() const {
  return m_invalidColumns;
}

template <typename Row>
boost::optional<Row> const &RowValidationResult<Row>::validRowElseNone() const {
  return m_validRow;
}

template class RowValidationResult<SingleRow>;
template class RowValidationResult<SlicedRow>;

boost::optional<std::vector<std::string>>
parseRunNumbers(std::string const &runNumbers) {
  return boost::none;
}

boost::optional<double> parseTheta(std::string const &theta) {
  return boost::none;
}

boost::optional<std::pair<std::string, std::string>>
parseTransmissionRuns(std::string const &transmissionRuns) {
  return boost::none;
}

template <typename Row>
RowValidationResult<Row> validateRow(std::vector<std::string> const &cellText) {
  return RowValidationResult<Row>({0, 1});
}

template RowValidationResult<SingleRow> validateRow(std::vector<std::string> const&);
template RowValidationResult<SlicedRow> validateRow(std::vector<std::string> const&);
}
}
