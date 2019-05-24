// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidQtWidgets/MplCpp/ColorConverter.h"

#include <QColor>

using Mantid::PythonInterface::GlobalInterpreterLock;
using MantidQt::Widgets::MplCpp::ColorConverter;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Contruct a wrapper around an existing matplotlib.lines.Line2D instance
 * This object owns the data that is part of the Line2D instance. It assumes the
 * Line2D contains numpy arrays that are simple a view on to the existing data
 * that is part of the given Y and X arrays.
 * @param obj An existing Line2D instance
 * @param xdataOwner The source data for X. It is moved into this object
 * @param ydataOwner The source data for Y. It is moved into this object
 */
Line2D::Line2D(Python::Object obj, std::vector<double> xdataOwner,
               std::vector<double> ydataOwner)
    : Artist(std::move(obj)), m_xOwner(std::move(xdataOwner)),
      m_yOwner(std::move(ydataOwner)) {
  assert(!m_xOwner.empty());
  assert(!m_yOwner.empty());
}

/**
 * The data is being deleted so the the line is removed from the axes
 * if it is present
 */
Line2D::~Line2D() noexcept {
  // If the Line2D has not been gutted by a std::move() then
  // detach the line
  if (!m_xOwner.empty()) {
    try {
      this->remove();
    } catch (...) {
      // line is not attached to an axes
    }
  }
}

/**
 * @return A QColor defining the color of the artist
 */
QColor Line2D::getColor() const {
  GlobalInterpreterLock lock;
  return ColorConverter::toRGB(pyobj().attr("get_color")());
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
