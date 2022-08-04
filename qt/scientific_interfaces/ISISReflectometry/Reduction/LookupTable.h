// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "Common/DllConfig.h"
#include "GUI/Preview/ROIType.h"
#include "LookupRow.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <exception>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class Row;
class PreviewRow;

struct RowNotFoundException : public std::out_of_range {
public:
  RowNotFoundException(std::string s) : std::out_of_range(std::move(s)){};
};

struct MultipleRowsFoundException : public std::length_error {
public:
  MultipleRowsFoundException(std::string s) : std::length_error(std::move(s)){};
};

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupTable {
public:
  LookupTable() = default;
  LookupTable(std::vector<LookupRow> rowsIn);
  LookupTable(std::initializer_list<LookupRow> rowsIn);

  boost::optional<LookupRow> findLookupRow(Row const &row, double tolerance) const;
  boost::optional<LookupRow> findLookupRow(PreviewRow const &previewRow, double tolerance) const;
  boost::optional<LookupRow> findWildcardLookupRow() const;
  void replaceLookupRow(PreviewRow const &previewRow, double tolerance);
  size_t getIndex(LookupRow const &) const;
  std::vector<LookupRow> const &rows() const;
  std::vector<LookupRow::ValueArray> toValueArray() const;

  friend bool operator==(LookupTable const &lhs, LookupTable const &rhs);
  friend bool operator!=(LookupTable const &lhs, LookupTable const &rhs);

private:
  std::vector<LookupRow> m_lookupRows;

  boost::optional<LookupRow> searchByTheta(std::vector<LookupRow> lookupRows, boost::optional<double> const &,
                                           double) const;
  std::vector<LookupRow> searchByTitle(Row const &row) const;
  std::vector<LookupRow> findMatchingRegexes(std::string const &title) const;
  std::vector<LookupRow> findEmptyRegexes() const;

  void replaceLookupRowProcessingInstructions(PreviewRow const &previewRow, LookupRow &lookupRow, ROIType regionType);
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
