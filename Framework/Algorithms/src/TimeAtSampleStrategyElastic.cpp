// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

/** Constructor
 */
TimeAtSampleStrategyElastic::TimeAtSampleStrategyElastic(
    Mantid::API::MatrixWorkspace_const_sptr ws)
    : m_ws(ws), m_spectrumInfo(m_ws->spectrumInfo()),
      m_beamDir(
          m_ws->getInstrument()->getReferenceFrame()->vecPointingAlongBeam()) {}

/**
 * @brief Calculate correction
 * @param workspace_index
 * @return Correction
 */
Correction
TimeAtSampleStrategyElastic::calculate(const size_t &workspace_index) const {

  // Calculate TOF ratio
  const double L1s = m_spectrumInfo.l1();
  double scale;
  if (m_spectrumInfo.isMonitor(workspace_index)) {
    double L1m =
        m_beamDir.scalar_prod(m_spectrumInfo.sourcePosition() -
                              m_spectrumInfo.position(workspace_index));
    scale = std::abs(L1s / L1m);
  } else {
    scale = L1s / (L1s + m_spectrumInfo.l2(workspace_index));
  }

  return Correction(0., scale);
}

} // namespace Algorithms
} // namespace Mantid
