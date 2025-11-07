// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/TimeAtSampleStrategy.h"
#include "MantidKernel/V3D.h"
#include <memory>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;

/** TimeAtSampleStrategyElastic : Time at sample stragegy for elastic scattering
 */
class MANTID_API_DLL TimeAtSampleStrategyElastic : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyElastic(std::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  const API::SpectrumInfo &m_spectrumInfo;
  const double m_L1s;
  const Kernel::V3D m_beamDir;
};

} // namespace API
} // namespace Mantid
