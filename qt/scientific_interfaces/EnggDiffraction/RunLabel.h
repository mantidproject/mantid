// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRUNLABEL_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFRUNLABEL_H_

#include "DllConfig.h"
#include <cstddef>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/// POD class for containing run number and bank ID used to index runs loaded
/// into the ED GUI
class MANTIDQT_ENGGDIFFRACTION_DLL RunLabel {
public:
  RunLabel(const std::string &runNumber, const size_t bank);

  RunLabel() = default;

  std::string runNumber;
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
