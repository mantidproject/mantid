// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../inc/MantidQtWidgets/Icon.h"

#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace Widgets {
namespace Icons {

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
  auto splitValues = splitIconName(iconNames[vectorIndex].toStrdString());
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

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt