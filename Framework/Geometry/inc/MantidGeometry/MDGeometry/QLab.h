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

/** QLab : Q in the lab frame MDFrame.
 */
class MANTID_GEOMETRY_DLL QLab : public MDFrame {
public:
  QLab();
  Mantid::Kernel::UnitLabel getUnitLabel() const override;
  const Mantid::Kernel::MDUnit &getMDUnit() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Mantid::Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  std::string name() const override;
  QLab *clone() const override;
  Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const override;

  // Type name
  static const std::string QLabName;

private:
  /// Fixed to be inverse angstroms
  const std::unique_ptr<const Mantid::Kernel::MDUnit> m_unit;
};

} // namespace Geometry
} // namespace Mantid
