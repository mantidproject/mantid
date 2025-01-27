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

/** HKL : HKL MDFrame
 */
class MANTID_GEOMETRY_DLL HKL : public MDFrame {
public:
  HKL(const HKL &other);
  HKL &operator=(const HKL &other);
  HKL(std::unique_ptr<Kernel::MDUnit> &unit);
  HKL(Kernel::MDUnit *unit);
  static const std::string HKLName;

  // MDFrame interface
  Kernel::UnitLabel getUnitLabel() const override;
  const Kernel::MDUnit &getMDUnit() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  std::string name() const override;
  HKL *clone() const override;
  Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const override;

private:
  std::unique_ptr<Kernel::MDUnit> m_unit;
};

} // namespace Geometry
} // namespace Mantid
