#ifndef MANTID_ISISREFLECTOMETRY_TRASNFERRESULTS_H_
#define MANTID_ISISREFLECTOMETRY_TRASNFERRESULTS_H_

#include "DllConfig.h"
#include <boost/make_shared.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_ISISREFLECTOMETRY_DLL TransferResults {
public:
  using COLUMN_NAME_TYPE = std::string;
  using COLUMN_VALUE_TYPE = std::string;

  using COLUMN_MAP_TYPE = std::map<COLUMN_NAME_TYPE, COLUMN_VALUE_TYPE>;

  TransferResults(std::vector<COLUMN_MAP_TYPE> transferRuns,
                  std::vector<COLUMN_MAP_TYPE> errorRuns);

  std::vector<COLUMN_MAP_TYPE> getTransferRuns();
  std::vector<COLUMN_MAP_TYPE> getErrorRuns();

  void addTransferRow(const COLUMN_MAP_TYPE &row);
  void addErrorRow(COLUMN_NAME_TYPE id, COLUMN_VALUE_TYPE error);

  std::vector<COLUMN_MAP_TYPE> m_transferRuns;
  std::vector<COLUMN_MAP_TYPE> m_errorRuns;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_ISISREFLECTOMETRY_TRASNFERRESULTS_H_!
