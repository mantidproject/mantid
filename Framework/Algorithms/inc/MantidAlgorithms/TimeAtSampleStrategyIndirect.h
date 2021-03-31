// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include <memory>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;
} // namespace API

namespace Algorithms {

/** TimeAtSampleStrategyIndirect : Determine Time At Sample for an indirect
  instrument setup.
*/
class MANTID_ALGORITHMS_DLL TimeAtSampleStrategyIndirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyIndirect(std::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Workspace to operate on
  std::shared_ptr<const Mantid::API::MatrixWorkspace> m_ws;
  const API::SpectrumInfo &m_spectrumInfo;
};

} // namespace Algorithms
} // namespace Mantid
