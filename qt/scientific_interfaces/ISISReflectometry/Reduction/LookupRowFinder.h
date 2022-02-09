// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "Common/DllConfig.h"
#include "LookupRow.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupRowFinder {
public:
  LookupRowFinder(LookupTable const &);

  LookupRow const *operator()(const boost::optional<double> &thetaAngle, double tolerance) const;

private:
  LookupTable const &m_lookupTable;

  LookupRow const *searchByTheta(const boost::optional<double> &thetaAngle, double tolerance) const;
  LookupRow const *searchForWildcard() const;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
