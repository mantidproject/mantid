// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"

#include <functional>
#include <optional>
#include <vector>

using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::V3D;

namespace Mantid {
namespace API {
class MANTID_API_DLL PanelsSurfaceCalculator {
public:
  void setupBasisAxes(const V3D &zaxis, V3D &xaxis, V3D &yaxis) const;
  std::vector<V3D> retrievePanelCorners(const ComponentInfo &componentInfo, const size_t rootIndex) const;
  V3D calculatePanelNormal(const std::vector<V3D> &panelCorners) const;
  bool isBankFlat(const ComponentInfo &componentInfo, size_t bankIndex, const std::vector<size_t> &tubes,
                  const V3D &normal);
  V3D calculateBankNormal(const ComponentInfo &componentInfo, const std::vector<size_t> &tubes);
  void setBankVisited(const ComponentInfo &componentInfo, size_t bankIndex, std::vector<bool> &visitedComponents) const;
  size_t findNumDetectors(const ComponentInfo &componentInfo, const std::vector<size_t> &components) const;
  Mantid::Kernel::Quat calcBankRotation(const V3D &detPos, V3D normal, const V3D &zAxis, const V3D &yAxis,
                                        const V3D &samplePosition) const;
  std::vector<Mantid::Kernel::V2D> transformedBoundingBoxPoints(const ComponentInfo &componentInfo,
                                                                size_t detectorIndex, const V3D &refPos,
                                                                const Mantid::Kernel::Quat &rotation, const V3D &xaxis,
                                                                const V3D &yaxis) const;
  std::vector<size_t> tubeDetectorParentIDs(const ComponentInfo &componentInfo, size_t rootIndex,
                                            std::vector<bool> &visited);
  std::vector<std::vector<size_t>> examineAllComponents(
      const ComponentInfo &componentInfo,
      std::function<std::vector<size_t>(const ComponentInfo &, size_t, std::vector<bool> &)> operation);
  std::optional<Kernel::V2D> getSideBySideViewPos(const ComponentInfo &componentInfo,
                                                  const Instrument_const_sptr &instrument,
                                                  const size_t componentIndex) const;

private:
  Mantid::Kernel::Logger g_log = Mantid::Kernel::Logger("PanelsSurfaceCalculator");
};
} // namespace API
} // namespace Mantid
