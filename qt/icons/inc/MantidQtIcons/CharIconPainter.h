// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <QIcon>
#include <QPainter>

namespace MantidQt {
namespace Icons {

class IconicFont;

/**
 * CharIconPainter is intended to be owned by an IconicFont object, this object
 * will pass it as a pointer to each of it's created CharIconEngines. The
 * functions inside of this object will perform the painting of the icon to the
 * pixmap inside of the QIcon object, when called by the CharIconEngine.
 *
 * QIcon calls pixmap/paint on CharIconEngine. CharIconEngine calls paint on
 * CharIconPainter.
 */
class EXPORT_OPT_MANTIDQT_ICONS CharIconPainter : public QPainter {
public:
  void paint(IconicFont *iconic, QPainter *painter, QRect rect, QIcon::Mode mode, QIcon::State state,
             QList<QHash<QString, QVariant>> &options);

private:
  void paintIcon(IconicFont *iconic, QPainter *painter, QRect rect, QIcon::Mode mode, QIcon::State state,
                 QHash<QString, QVariant> &options);
};

} // namespace Icons
} // namespace MantidQt
