// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/TimeAtSampleStrategy.h"
#include <memory>

namespace Mantid {

namespace API {
class MatrixWorkspace;

/** TimeAtSampleStrategyDirect : Determine the Time at Sample corrections for a
  Direct Geometry instrument
*/
class MANTID_API_DLL TimeAtSampleStrategyDirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyDirect(const std::shared_ptr<const Mantid::API::MatrixWorkspace> &ws, double ei);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Cached L1, Ei dependent const shift
  double m_constShift;
};

} // namespace API
} // namespace Mantid
