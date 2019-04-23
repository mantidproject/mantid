// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_
#define MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_

#include "DllOption.h"

#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QVariant>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace Icons {

class IconicFont;
class CharIconPainter;

class EXPORT_OPT_MANTIDQT_ICONS CharIconEngine : public QIconEngine {
public:
  CharIconEngine(IconicFont *iconic, CharIconPainter *painter,
                 std::vector<QHash<QString, QVariant>> &options);
  void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
             QIcon::State state) override;
  QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                 QIcon::State state) override;
  QIconEngine *clone() const override;

private:
  IconicFont *m_iconic;
  CharIconPainter *m_painter;
  std::vector<QHash<QString, QVariant>> &m_options;
};

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_ */