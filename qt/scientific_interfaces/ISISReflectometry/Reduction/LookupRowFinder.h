// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "LookupRow.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class LookupRowFinder {
public:
  LookupRowFinder(LookupTable const &);

  LookupRow const *operator()(const boost::optional<double> &thetaAngle, double tolerance) const;

private:
  LookupTable const &m_lookupTable;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
