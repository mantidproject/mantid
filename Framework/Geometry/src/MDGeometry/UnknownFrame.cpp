// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/UnknownFrame.h"

namespace Mantid {
namespace Geometry {

UnknownFrame::UnknownFrame(std::unique_ptr<Kernel::MDUnit> unit)
    : m_unit(unit.release()) {}

UnknownFrame::UnknownFrame(const Kernel::UnitLabel &unit)
    : m_unit(new Mantid::Kernel::LabelUnit(unit)) {}

const std::string UnknownFrame::UnknownFrameName = "Unknown frame";

bool UnknownFrame::canConvertTo(
    const Mantid::Kernel::MDUnit & /*otherUnit*/) const {
  return false; // Cannot convert since it is unknown
}

bool UnknownFrame::setMDUnit(const Mantid::Kernel::MDUnit & /*newUnit*/) {
  return false;
}

std::string UnknownFrame::name() const { return UnknownFrameName; }

Mantid::Kernel::UnitLabel UnknownFrame::getUnitLabel() const {
  return m_unit->getUnitLabel();
}

const Mantid::Kernel::MDUnit &UnknownFrame::getMDUnit() const {
  return *m_unit;
}

Mantid::Kernel::SpecialCoordinateSystem
UnknownFrame::equivalientSpecialCoordinateSystem() const {
  return Mantid::Kernel::SpecialCoordinateSystem::None;
}

UnknownFrame *UnknownFrame::clone() const {
  return new UnknownFrame(std::unique_ptr<Kernel::MDUnit>(m_unit->clone()));
}

bool UnknownFrame::isQ() const { return false; }

bool UnknownFrame::isSameType(const MDFrame &frame) const {
  auto isSameType = true;
  try {
    const auto &tmp = dynamic_cast<const UnknownFrame &>(frame);
    UNUSED_ARG(tmp);
  } catch (std::bad_cast &) {
    isSameType = false;
  }
  return isSameType;
}
} // namespace Geometry
} // namespace Mantid
