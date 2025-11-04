// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace Icons {

class IconicFont;
class CharIconPainter;

/**
 * This Class is used to paint the icon onto the pixmap inside of a QIcon
 * object. It makes a custom call to a painter named CharIconPainter, this
 * occurs both via the paint and pixmap functions on the objects made with this
 * class.
 *
 * The ownership of this object should be taken by a QIcon that recieves this as
 * it's constructor.
 */
class EXPORT_OPT_MANTIDQT_ICONS CharIconEngine : public QIconEngine {
public:
  CharIconEngine(IconicFont *iconic, CharIconPainter *const painter, const QList<QHash<QString, QVariant>> &options);
  void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
  QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
  QIconEngine *clone() const override;

private:
  IconicFont *m_iconic;
  CharIconPainter *m_painter;
  QList<QHash<QString, QVariant>> m_options;
};

} // namespace Icons
} // namespace MantidQt
