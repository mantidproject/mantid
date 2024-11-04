// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidQtIcons/CharIconEngine.h"
#include "MantidQtIcons/CharIconPainter.h"

namespace MantidQt::Icons {

CharIconEngine::CharIconEngine(IconicFont *iconic, CharIconPainter *painter,
                               const QList<QHash<QString, QVariant>> &options)
    : m_iconic(iconic), m_painter(painter), m_options(options) {}

void CharIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) {
  m_painter->paint(m_iconic, painter, rect, mode, state, m_options);
}

QPixmap CharIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) {
  QPixmap pmap(size);
  pmap.fill(Qt::transparent);
  QPainter painter(&pmap);
  this->paint(&painter, QRect(QPoint(0, 0), size), mode, state);
  return pmap;
}

QIconEngine *CharIconEngine::clone() const { return new CharIconEngine(m_iconic, m_painter, m_options); }

} // namespace MantidQt::Icons
