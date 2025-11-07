// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TimeAtSampleStrategyElastic.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid::API {

namespace {
constexpr double ZERO_ADDITIVE_OFFSET{0.};
}

/** Constructor
 */
TimeAtSampleStrategyElastic::TimeAtSampleStrategyElastic(Mantid::API::MatrixWorkspace_const_sptr ws)
    : m_spectrumInfo(ws->spectrumInfo()), m_L1s(m_spectrumInfo.l1()),
      m_beamDir(ws->getInstrument()->getReferenceFrame()->vecPointingAlongBeam()) {}

/**
 * @brief Calculate correction
 * @param workspace_index
 * @return Correction
 */
Correction TimeAtSampleStrategyElastic::calculate(const size_t &workspace_index) const {

  // Calculate TOF ratio
  double scale;
  if (m_spectrumInfo.isMonitor(workspace_index)) {
    double L1m = m_beamDir.scalar_prod(m_spectrumInfo.sourcePosition() - m_spectrumInfo.position(workspace_index));
    scale = std::abs(m_L1s / L1m);
  } else {
    scale = m_L1s / (m_L1s + m_spectrumInfo.l2(workspace_index));
  }

  return Correction(scale, ZERO_ADDITIVE_OFFSET);
}

} // namespace Mantid::API
