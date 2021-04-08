// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "MantidKernel/MDUnit.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/UnitLabel.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/** MDFrame : The coordinate frame for a dimension, or set of dimensions in a
  multidimensional workspace.
*/
class DLLExport MDFrame {
public:
  virtual Mantid::Kernel::UnitLabel getUnitLabel() const = 0;
  virtual const Mantid::Kernel::MDUnit &getMDUnit() const = 0;
  virtual bool setMDUnit(const Mantid::Kernel::MDUnit &newUnit) = 0;
  virtual bool canConvertTo(const Mantid::Kernel::MDUnit &otherUnit) const = 0;
  virtual bool isQ() const = 0;
  virtual bool isSameType(const MDFrame &frame) const = 0;
  virtual std::string name() const = 0;
  virtual Mantid::Kernel::SpecialCoordinateSystem equivalientSpecialCoordinateSystem() const = 0;
  virtual MDFrame *clone() const = 0;
  virtual ~MDFrame() = default;
};

using MDFrame_uptr = std::unique_ptr<MDFrame>;
using MDFrame_const_uptr = std::unique_ptr<const MDFrame>;
using MDFrame_sptr = std::shared_ptr<MDFrame>;
using MDFrame_const_sptr = std::shared_ptr<const MDFrame>;

} // namespace Geometry
} // namespace Mantid
