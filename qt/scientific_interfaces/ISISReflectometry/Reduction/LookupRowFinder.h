// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "Common/DllConfig.h"
#include "LookupRow.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class Row;

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRowFinder {
public:
  LookupRowFinder(LookupTable const &);

  boost::optional<LookupRow> operator()(Row const &row, double tolerance) const;
  boost::optional<LookupRow> findWildcardLookupRow() const;

private:
  LookupTable const &m_lookupTable;

  boost::optional<LookupRow> searchByTheta(std::vector<LookupRow> lookupRows, boost::optional<double> const &,
                                           double) const;
  std::vector<LookupRow> searchByTitle(Row const &row) const;
  std::vector<LookupRow> findMatchingRegexes(std::string const &title) const;
  std::vector<LookupRow> findEmptyRegexes() const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
