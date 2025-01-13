// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** GeneralFrame : Any MDFrame that isn't related to momemtum transfer
 */
class MANTID_GEOMETRY_DLL GeneralFrame : public MDFrame {
public:
  static const std::string GeneralFrameDistance;
  static const std::string GeneralFrameTOF;
  static const std::string GeneralFrameName;
  GeneralFrame(std::string frameName, const Kernel::UnitLabel &unit);
  GeneralFrame(std::string frameName, std::unique_ptr<Mantid::Kernel::MDUnit> unit);
  Kernel::UnitLabel getUnitLabel() const override;
  const Kernel::MDUnit &getMDUnit() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  std::string name() const override;
  GeneralFrame *clone() const override;
  Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const override;

private:
  /// Label unit
  std::unique_ptr<Mantid::Kernel::MDUnit> m_unit;
  /// Frame name
  const std::string m_frameName;
};

} // namespace Geometry
} // namespace Mantid
