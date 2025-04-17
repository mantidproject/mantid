// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "Common/DllConfig.h"
#include "LookupRow.h"
#include <boost/regex.hpp>
#include <optional>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class Row;
class PreviewRow;

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable {
public:
  LookupTable() = default;
  LookupTable(std::vector<LookupRow> rowsIn);
  LookupTable(std::initializer_list<LookupRow> rowsIn);

  std::optional<LookupRow> findLookupRow(Row const &row, double tolerance) const;
  std::optional<LookupRow> findLookupRow(PreviewRow const &previewRow, double tolerance) const;
  std::optional<LookupRow> findWildcardLookupRow() const;
  void updateLookupRow(LookupRow lookupRow, double tolerance);
  size_t getIndex(LookupRow const &) const;
  std::vector<LookupRow> const &rows() const;
  std::vector<LookupRow::ValueArray> toValueArray() const;

  friend bool operator==(LookupTable const &lhs, LookupTable const &rhs);
  friend bool operator!=(LookupTable const &lhs, LookupTable const &rhs);

private:
  std::vector<LookupRow> m_lookupRows;

  std::optional<LookupRow> findLookupRow(std::string const &title, std::optional<double> const &, double) const;
  std::optional<LookupRow> searchByTheta(std::vector<LookupRow> lookupRows, std::optional<double> const &,
                                         double) const;
  std::vector<LookupRow> findMatchingRegexes(std::string const &title) const;
  std::vector<LookupRow> findEmptyRegexes() const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
