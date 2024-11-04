// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <unordered_set>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL InvalidLookupRowCells {
public:
  InvalidLookupRowCells(int row, std::unordered_set<int> invalidColumns);
  std::unordered_set<int> const &invalidColumns() const;
  int row() const;

private:
  std::unordered_set<int> m_invalidColumns;
  int m_row;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(InvalidLookupRowCells const &lhs, InvalidLookupRowCells const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(InvalidLookupRowCells const &lhs, InvalidLookupRowCells const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
