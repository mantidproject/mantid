// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/CSGObject.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

using namespace Mantid::Geometry;

namespace MantidQt::MantidWidgets {

UnwrappedDetector::UnwrappedDetector()
    : color(GLColor(0, 0, 0)), u(0), v(0), width(0), height(0), uscale(0), vscale(0) {}

UnwrappedDetector::UnwrappedDetector(const GLColor &color, size_t detIndex)
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detIndex(detIndex) {
  this->color = color;
}

/** Copy constructor */
UnwrappedDetector::UnwrappedDetector(const UnwrappedDetector &other) { this->operator=(other); }

/** Assignment operator */
UnwrappedDetector &UnwrappedDetector::operator=(const UnwrappedDetector &other) = default;

bool UnwrappedDetector::empty() const { return detIndex == std::numeric_limits<size_t>::max(); }

QRectF UnwrappedDetector::toQRectF() const {
  return QRectF(QPointF(u - width / 2, v - height / 2), QSizeF(width, height));
}

} // namespace MantidQt::MantidWidgets
