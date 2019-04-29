// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/MDUnit.h"

namespace Mantid {
namespace Geometry {

const std::string QSample::QSampleName = "QSample";

//----------------------------------------------------------------------------------------------
/** Constructor
 */
QSample::QSample() : m_unit(new Mantid::Kernel::InverseAngstromsUnit) {}

Kernel::UnitLabel QSample::getUnitLabel() const {
  return m_unit->getUnitLabel();
}

const Kernel::MDUnit &QSample::getMDUnit() const { return *m_unit; }

bool QSample::setMDUnit(const Mantid::Kernel::MDUnit & /*newUnit*/) {
  return false;
}

bool QSample::canConvertTo(const Kernel::MDUnit &otherUnit) const {
  return this->getMDUnit() == otherUnit;
}

std::string QSample::name() const { return QSampleName; }

QSample *QSample::clone() const { return new QSample; }

Mantid::Kernel::SpecialCoordinateSystem
QSample::equivalientSpecialCoordinateSystem() const {
  return Mantid::Kernel::SpecialCoordinateSystem::QSample;
}

bool QSample::isQ() const { return true; }

bool QSample::isSameType(const MDFrame &frame) const {
  auto isSameType = true;
  try {
    const auto &tmp = dynamic_cast<const QSample &>(frame);
    UNUSED_ARG(tmp);
  } catch (std::bad_cast &) {
    isSameType = false;
  }
  return isSameType;
}

} // namespace Geometry
} // namespace Mantid
