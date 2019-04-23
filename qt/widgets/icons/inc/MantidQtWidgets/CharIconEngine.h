// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_
#define MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_

#include <QHash>
#include <QIcon>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QVariant>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace Icons {

class IconicFont;

class CharIconEngine : QIconEngine {
public:
  CharIconEngine::CharIconEngine(
      IconicFont &iconic, QPainter *painter,
      std::vector<QHash<QString, QVariant>> &options);
  void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
             QIcon::State state) override;
  void pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);

private:
  IconicFont &m_iconic;
  QPainter *m_painter;
  std::vector<QHash<QString, QVariant>> &m_options;
};

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_CHARICONENGINE_H_ */