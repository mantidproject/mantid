// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** UnknownFrame : Unknown MDFrame
 */
class MANTID_GEOMETRY_DLL UnknownFrame : public MDFrame {
public:
  UnknownFrame(std::unique_ptr<Kernel::MDUnit> unit);
  UnknownFrame(const Kernel::UnitLabel &unit);
  std::string name() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Mantid::Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  Mantid::Kernel::UnitLabel getUnitLabel() const override;
  const Mantid::Kernel::MDUnit &getMDUnit() const override;
  Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const override;
  UnknownFrame *clone() const override;
  // Type name
  static const std::string UnknownFrameName;

private:
  /// Label unit
  const std::unique_ptr<Mantid::Kernel::MDUnit> m_unit;
};

} // namespace Geometry
} // namespace Mantid
