// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_
#define MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_

// Std imports
#include <vector>

// Third party imports
#include <QHash>
#include <QIcon>
#include <QIconEngine>
#include <QString>
#include <QVariant>
#include <boost/optional.hpp>

QIcon getIcon(const QString &iconName);

QIcon getIcon(const QString &iconName,
              const boost::optional<QString> &color = boost::none_t,
              const boost::optional<double> &scaleFactor = boost::none_t);

QIcon getIcon(const std::vector<QString> &iconNames,
              const boost::optional<std::vector<QHash<QString, QVariant>>>
                  &options = boost::none_t);

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

class CharIconPainter {
public:
  void paint(IconicFont &iconic, QPainter *painter, QRect rect,
             QIcon::Mode mode, QIcon::State state,
             std::vector<QHash<QString, QVariant>> &options);

private:
  void paintIcon(IconicFont &iconic, QPainter *painter, QRect rect,
                 QIcon::Mode mode, QIcon::State state,
                 QHash<QString, QVariant> &options);
};

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

#endif /* MANTIDQT_WIDGETS_ICONS_ICONLOADER_H_ */