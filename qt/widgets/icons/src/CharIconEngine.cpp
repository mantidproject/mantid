// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../inc/MantidQtWidgets/CharIconEngine.h"

namespace MantidQt {
namespace Widgets {
namespace Icons {

CharIconEngine::CharIconEngine(IconicFont &iconic, QPainter *painter,
                               std::vector<QHash<QString, QVariant>> &options)
    : m_iconic(iconic), m_painter(painter), m_options(options) {}

void CharIconEngine::paint(QPainter *painter, const QRect &rect,
                           QIcon::Mode mode, QIcon::State state) {
  m_painter->paint(m_iconic, painter, rect, mode, state, m_options);
}

void CharIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                            QIcon::State state) {
  pmap = QPixmap(size);
  pmap.fill(Qt.transparent);
  this->paint(QPainter(pm), QRect(QPoint(0, 0), size), mode, state);
  return pmap;
}

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt