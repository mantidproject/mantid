// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_INVALIDDEFAULTSERROR_H
#define MANTID_ISISREFLECTOMETRY_INVALIDDEFAULTSERROR_H
#include "Common/DllConfig.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL InvalidDefaultsError {
public:
  InvalidDefaultsError(int row, std::vector<int> invalidColumns);
  std::vector<int> const &invalidColumns() const;
  int row() const;

private:
  std::vector<int> m_invalidColumns;
  int m_row;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(InvalidDefaultsError const &lhs,
                                               InvalidDefaultsError const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(InvalidDefaultsError const &lhs,
                                               InvalidDefaultsError const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INVALIDDEFAULTSERROR_H
