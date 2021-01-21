// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL InvalidDefaultsError {
public:
  InvalidDefaultsError(int row, std::vector<int> invalidColumns);
  std::vector<int> const &invalidColumns() const;
  int row() const;

private:
  std::vector<int> m_invalidColumns;
  int m_row;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(InvalidDefaultsError const &lhs, InvalidDefaultsError const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(InvalidDefaultsError const &lhs, InvalidDefaultsError const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt