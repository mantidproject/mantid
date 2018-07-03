#ifndef MANTID_CUSTOMINTERFACES_VALIDATIONRESULT_H_
#define MANTID_CUSTOMINTERFACES_VALIDATIONRESULT_H_
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row>
class MANTIDQT_ISISREFLECTOMETRY_DLL ValidationResult {
public:
  explicit ValidationResult(Row row);
  explicit ValidationResult(std::vector<int> invalidColumns);

  bool isValid() const;
  std::vector<int> const &invalidColumns() const;
  boost::optional<Row> const &validRowElseNone() const;

private:
  std::vector<int> m_invalidColumns;
  boost::optional<Row> m_validRow;
};

template <typename Row>
ValidationResult<Row>::ValidationResult(Row row)
    : m_invalidColumns(), m_validRow(std::move(row)) {}

template <typename Row>
ValidationResult<Row>::ValidationResult(std::vector<int> invalidColumns)
    : m_invalidColumns(std::move(invalidColumns)), m_validRow(boost::none) {}

template <typename Row> bool ValidationResult<Row>::isValid() const {
  return m_validRow.is_initialized();
}

template <typename Row>
std::vector<int> const &ValidationResult<Row>::invalidColumns() const {
  return m_invalidColumns;
}

template <typename Row>
boost::optional<Row> const &ValidationResult<Row>::validRowElseNone() const {
  return m_validRow;
}

}
}
#endif // MANTID_CUSTOMINTERFACES_VALIDATIONRESULT_H_
