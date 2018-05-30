#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRUNLABEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRUNLABEL_H_

#include "DllConfig.h"

#include <cstddef>

namespace MantidQt {
namespace CustomInterfaces {

/// POD class for containing run number and bank ID used to index runs loaded
/// into the ED GUI
class MANTIDQT_ENGGDIFFRACTION_DLL RunLabel {
public:
  RunLabel(const int runNumber, const size_t bank);

  RunLabel() = default;

  int runNumber;
  std::size_t bank;
};

MANTIDQT_ENGGDIFFRACTION_DLL bool operator==(const RunLabel &lhs,
                                             const RunLabel &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool operator!=(const RunLabel &lhs,
                                             const RunLabel &rhs);

MANTIDQT_ENGGDIFFRACTION_DLL bool operator<(const RunLabel &lhs,
                                            const RunLabel &rhs);

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRUNLABEL_H_
