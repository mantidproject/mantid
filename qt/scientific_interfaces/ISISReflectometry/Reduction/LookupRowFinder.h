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

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRowFinder {
public:
  LookupRowFinder(LookupTable const &);

  boost::optional<LookupRow> operator()(boost::optional<double> const &thetaAngle, double tolerance,
                                        std::string const & = "") const;

private:
  LookupTable const &m_lookupTable;

  boost::optional<LookupRow> searchByTheta(boost::optional<double> const &, double) const;
  std::vector<LookupRow> searchByTitle(std::string_view title) const;
  boost::optional<LookupRow> searchForWildcard() const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
