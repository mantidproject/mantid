// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/V3D.h"
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TimeAtSampleStrategyDirect::TimeAtSampleStrategyDirect(
    MatrixWorkspace_const_sptr ws, double ei)
    : m_constShift(0) {

  // A constant among all spectra
  constexpr double TWO_MEV_OVER_MASS =
      2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;

  // Get L1
  const auto &samplepos = ws->getInstrument()->getSample()->getPos();
  const auto &sourcepos = ws->getInstrument()->getSource()->getPos();
  double l1 = samplepos.distance(sourcepos);

  // Calculate constant (to all spectra) shift
  m_constShift = l1 / std::sqrt(ei * TWO_MEV_OVER_MASS);
}

/**
 * @brief Calculate corrections to get a Time at Sample for a DG instrument.
 * @return Correction struct
 */
Correction Mantid::Algorithms::TimeAtSampleStrategyDirect::calculate(
    const size_t & /*workspace_index*/) const {

  // Correction is L1 and Ei dependent only. Detector positions are not
  // required.
  return Correction(0, m_constShift);
}
} // namespace Algorithms
} // namespace Mantid
