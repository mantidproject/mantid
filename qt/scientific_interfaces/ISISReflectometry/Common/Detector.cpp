// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Detector.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

bool hasLinearDetector(Mantid::API::MatrixWorkspace_sptr &ws) {
  // If it returns Full (e.g. a single rectangular detector), we do want to SumBanks
  // If returns Partial (e.g. a rectangular detector and some point detectors), we don't want to SumBanks
  // If returns None (e.g. no rectangular detectors), We don't want to SumBanks
  // See a full explanation along with assumptions in issue #34270
  auto const detectorState = ws->getInstrument()->containsRectDetectors();
  return detectorState != Mantid::Geometry::Instrument::ContainsState::Full;
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
