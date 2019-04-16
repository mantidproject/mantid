// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Icon.h"

#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <boost/algorithm/string.hpp>

namespace {
static IconicFont iconicFont;
IconicFont &iconFontInstance() { return &iconicFont; }

QHash<QString, QVariant> loadJsonFile(const QString &jsonFile) {
  QFile jsonFile(charmapFileName);
  jsonFile.open(QFile::ReadOnly);
  const auto jsonDocument = QJsonDocument().fromJson(jsonFile.readAll());
  const auto jsonObject = jsonDocument.object();
  // VariantHash = QHash<QString, QVariant> in this case QVaraint = QString
  return jsonObject.toVariantHash();
}

std::vector<std::string> splitIconName(std::string iconName) {
  std::vector<std::string> results;
  boost::split(results, iconName, [](char c) { return c == '.'; });

  if (results.size() != 2) {
    throw std::invalid_argument("Icon name passed is incorrect format");
  }
  return results;
}

void addValuesToOptions(std::vector<QHash<QString, QVariant>> &options,
                        const std::vector<QString> iconNames,
                        unsigned int vectorIndex) {
  auto splitValues = splitIconNames(iconNames[vectorIndex].toStrdString());
  options[vectorIndex].insert(QString("prefix"),
                              QVariant(QString(splitValues[0])));
  options[vectorIndex].insert(QString("charecter"),
                              QVariant(QString(splitValues[1])));
}
} // namespace

QIcon getIcon(const QString &iconName) { return getIcon({iconName}); }

QIcon getIcon(const QString &iconName, const boost::optional<QString> &color,
              const boost::optional<double> &scaleFactor) {
  QHash<QString, QVariant> options;
  if (color) {
    options.insert(QString("color"), QVariant(color.get()));
  }
  if (scaleFactor) {
    options.insert(QString("scaleFactor"), QVariant(scaleFactor.get()));
  }
  if (!options.empty()) {
    return getIcon({iconName}, {options});
  } else {
    return getIcon({iconName});
  }
}

QIcon getIcon(
    const std::vector<QString> &iconNames,
    const boost::optional<std::vector<QHash<QString, QVariant>>> &options) {
  auto iconicFont = iconFontInstance();
  return iconicFont.getIcon(iconNames, options);
}

IconicFont::IconicFont() {
  this->load_font(QString("mdi"), QString(":/mdi-font.ttf"),
                  QString(":/mdi-charmap.json"));
}

QIcon IconicFont::getIcon(
    const std::vector<QString> &iconNames,
    const boost::optional<std::vector<QHash<QString, QVariant>>> &options) {
  std::vector<QHash<QString, QVariant>> actualOptions;
  if (options) {
    // Assume there are mutliple
    if (iconNames.size() != options.get().size()) {
      throw std::invalid_argument(
          "Icon names passed and options are not the same length");
    }
    actualOptions = options;
    for (auto i = 0u; i < iconNames.size(); ++i) {
      addValuesToOptions(options, iconNames, i);
    }
  } else {
    // Assert that there is only one IconName
    if (iconNames.size() > 1) {
      throw std::invalid_argument("Extra Icon names passed without options");
    }
    addValuesToOptions(options, iconNames, 0);
  }
  return iconByPainter(m_painter, options);
}

QIcon IconicFont::iconByPainter(
    const QPainter *painter,
    const std::vector<QHash<QString, QVariant>> &options) {
  auto engine = new CharIconEngine(painter, options);
  return QIcon(engine);
}

void IconicFont::load_font(const QString &prefix, const QString &ttfFilename,
                           const QString &charmapFilename) {
  auto fontDatabase = QFontDatabase.instance();
  const auto id = fontDatabase.addApplicationFont(QFile(ttfFilename));
  const auto loadedFontFamilies = fontDatabase.applicationFontFamilies(id);
  if (loadedFontFamilies) {
    m_fontnames.insert(prefix, loadedFontFamilies[0]);
  }

  const auto hashValues = loadJsonFile(charmapFilename);
  m_charmap.insert(prefix, hashValues);
}

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

void CharIconPainter::paint(IconicFont &iconic, QPainter *painter, QRect rect,
                            QIcon::Mode mode, QIcon::State state,
                            std::vector<QHash<QString, QVariant>> &options) {
  for (auto &option : options) {
    this->paintIcon(iconic, painter, rect, mode, state, option);
  }
}

void CharIconPainter::paintIcon(IconicFont &iconic, QPainter *painter,
                                QRect rect, QIcon::Mode mode,
                                QIcon::State state,
                                QHash<QString, QVariant> &options) {
  painter->save();
  const auto colorVariant = options[QString("color")];
  const auto scaleVariant = options[QString("scaleFactor")];
  const auto charecterStringVariant = options[QString("charecter")];
  const auto prefixVariant = options[QString("prefix")];
  const auto charecter = iconic.getCharmap()[prefixVariant.toString()]
                                            [charecterStringVariant.toString()];

  // Set some defaults so it doesn't fail later if nothing was set
  QString color("black");
  double scaleFactor(1.0);
  if (colorVariant.isString()) {
    color = colorVariant.toString();
  }
  if (scaleVariant.isString()) {
    scaleFactor = charVariant.toString();
  }
  painter.setPen(QColor(color));

  // A 16 pixel-high icon yields a font size of 14, which is pixel perfect for
  // font-awesome. 16 * 0.875 = 14 The reason why the glyph size is smaller than
  // the icon size is to account for font bearing.
  drawSize = 0.875 * unsigned long(rect.height() * scaleFactor);

  painter->setFont(iconic.font(m_prefix, draw_size));
  painter->setOpacity(1.0);
  painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, charecter);
  painter->restore();
}