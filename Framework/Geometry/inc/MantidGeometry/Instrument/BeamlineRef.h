// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class InstrumentMetadata;

/** BeamlineRef : an instrument as Instrument 2.0 holds it.

  The ComponentInfo and DetectorInfo are cross-linked, so they are always held together. Both share one parameter
  store. The metadata is null if none was given.
 */
struct BeamlineRef {
  std::shared_ptr<const ComponentInfo> componentInfo;
  std::shared_ptr<const DetectorInfo> detectorInfo;
  std::shared_ptr<const InstrumentMetadata> metadata;
};

} // namespace Geometry
} // namespace Mantid
