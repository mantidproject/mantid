// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeBase.h"
#include <vector>

namespace Mantid {
namespace DataObjects {

/** PeakShapeDetectorBin : PeakShape representing detector ids and integration limits of a peak
 */
class MANTID_DATAOBJECTS_DLL PeakShapeDetectorBin : public PeakShapeBase {
public:
  PeakShapeDetectorBin(const std::vector<std::tuple<int32_t, double, double>> &detectorBinList,
                       Kernel::SpecialCoordinateSystem frame, std::string algorithmName = std::string(),
                       int algorithmVersion = -1);
  /// Equals operator
  bool operator==(const PeakShapeDetectorBin &other) const;

  /// PeakShape interface
  std::string toJSON() const override;
  /// Clone PeakShapeDetectorBin
  Mantid::Geometry::PeakShape *clone() const override;
  /// Get the peak shape
  std::string shapeName() const override;

  /// PeakBase interface
  std::optional<double> radius(RadiusType type = RadiusType::Radius) const override;

  static const std::string detectorBinShapeName();

  const std::vector<std::tuple<int32_t, double, double>> &getDetectorBinList() const { return m_detectorBinList; }

private:
  std::vector<std::tuple<int32_t, double, double>> m_detectorBinList;
};

using PeakShapeDetectorTOF_sptr = std::shared_ptr<PeakShapeDetectorBin>;
using PeakShapeDetectorTOF_const_sptr = std::shared_ptr<const PeakShapeDetectorBin>;

} // namespace DataObjects
} // namespace Mantid
