// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_
#define MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_

#include "CharIconEngine.h"
#include "CharIconPainter.h"

#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QString>
#include <QVariant>
#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace Icons {

QIcon getIcon(const QString &iconName);

QIcon getIcon(const QString &iconName,
              const boost::optional<QString> &color = boost::none,
              const boost::optional<double> &scaleFactor = boost::none);

QIcon getIcon(const std::vector<QString> &iconNames,
              const boost::optional<std::vector<QHash<QString, QVariant>>>
                  &options = boost::none);

class IconicFont {
public:
  IconicFont();
  QIcon getIcon(const std::vector<QString> &iconNames,
                const boost::optional<std::vector<QHash<QString, QVariant>>>
                    &options = boost::none_t);
  QHash<QString, QHash<QString, QVariant>> const getCharmap() const;

private:
  void load_font(const QString &prefix, const QString &ttfFilename,
                 const QString &charmapFilename);

  QHash<QString, QString> m_fontnames;

  // The QVariant will always be internally, a QString.
  QHash<QString, QHash<QString, QVariant>> m_charmap;

  CharIconPainter m_painter;
};

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_ */