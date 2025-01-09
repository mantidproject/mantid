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

/** QSample : Q in the sample frame
 */
class MANTID_GEOMETRY_DLL QSample : public MDFrame {
public:
  static const std::string QSampleName;
  QSample();
  Kernel::UnitLabel getUnitLabel() const override;
  const Kernel::MDUnit &getMDUnit() const override;
  bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) override;
  bool canConvertTo(const Kernel::MDUnit &otherUnit) const override;
  bool isQ() const override;
  bool isSameType(const MDFrame &frame) const override;
  std::string name() const override;
  QSample *clone() const override;
  Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const override;

private:
  /// immutable unit for qlab.
  const std::unique_ptr<const Mantid::Kernel::InverseAngstromsUnit> m_unit;
};

} // namespace Geometry
} // namespace Mantid
