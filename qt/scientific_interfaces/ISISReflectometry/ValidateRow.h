#ifndef MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
#include "Reduction/Row.h"
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

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

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::vector<std::string>>
parseRunNumbers(std::string const &runNumbers);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseTheta(std::string const &theta);

MANTIDQT_ISISREFLECTOMETRY_DLL
    boost::optional<std::pair<std::string, std::string>>
    parseTransmissionRuns(std::string const &transmissionRuns);

template <typename Row>
RowValidationResult<Row> validateRow(std::vector<std::string> const &cellText);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SlicedRow>;
extern template class MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SingleRow>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SlicedRow> validateRow(std::vector<std::string> const&);
extern template MANTIDQT_ISISREFLECTOMETRY_DLL RowValidationResult<SingleRow> validateRow(std::vector<std::string> const&);

}
}
#endif // MANTID_CUSTOMINTERFACES_VALIDATEROW_H_
