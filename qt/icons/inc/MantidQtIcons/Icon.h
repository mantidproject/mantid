// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ICONS_ICONLOADER_H_
#define MANTIDQT_ICONS_ICONLOADER_H_

#include "MantidQtIcons/CharIconEngine.h"
#include "MantidQtIcons/CharIconPainter.h"
#include "MantidQtIcons/DllOption.h"

#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QString>
#include <QVariant>
#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace Icons {

EXPORT_OPT_MANTIDQT_ICONS QIcon getIcon(const QString &iconName,
                                        const QString &color = QString("black"),
                                        const double &scaleFactor = 1.0);

EXPORT_OPT_MANTIDQT_ICONS QIcon getIcon(const QStringList &iconNames,
                                        const QList<QVariant> &options);

EXPORT_OPT_MANTIDQT_ICONS QIcon
getIcon(const QStringList &iconNames,
        const QList<QHash<QString, QVariant>> &options);

class EXPORT_OPT_MANTIDQT_ICONS IconicFont {
public:
  IconicFont();
  QIcon getIcon(const QStringList &iconNames,
                const QList<QHash<QString, QVariant>> &options);
  QString findCharacterFromCharMap(const QString &, const QString &) const;
  QFont getFont(const QString &prefix, const int drawSize);

private:
  void loadFont(const QString &prefix, const QString &ttfFilename,
                const QString &charmapFilename);
  QIcon iconByPainter(CharIconPainter *painter,
                      QList<QHash<QString, QVariant>> &options);
  void addValuesToOptions(QList<QHash<QString, QVariant>> &options,
                          const QStringList &iconNames,
                          unsigned int vectorIndex);

  QHash<QString, QString> m_fontnames;

  // The QVariant will always be internally, a QString.
  QHash<QString, QHash<QString, QVariant>> m_charmap;

  CharIconPainter m_painter;
};

} // namespace Icons
} // namespace MantidQt

#endif /* MANTIDQT_ICONS_ICONLOADER_H_ */