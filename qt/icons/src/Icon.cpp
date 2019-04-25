// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtIcons/Icon.h"

#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <boost/algorithm/string.hpp>

namespace {
MantidQt::Icons::IconicFont &iconFontInstance() {
  static MantidQt::Icons::IconicFont iconicFont;
  return iconicFont;
}

QHash<QString, QVariant> loadJsonFile(const QString &charmapFileName) {
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
  std::string iconName = iconNames[vectorIndex].toStdString();
  auto splitValues = splitIconName(iconName);
  const auto prefix = splitValues.at(0);
  const auto charecter = splitValues.at(1);
  options[vectorIndex].insert(QString("prefix"),
                              QVariant(QString::fromStdString(prefix)));
  options[vectorIndex].insert(QString("charecter"),
                              QVariant(QString::fromStdString(charecter)));
}
} // namespace

QIcon MantidQt::Icons::getIcon(const QString &iconName, const QString &color,
                               const double &scaleFactor) {
  QHash<QString, QVariant> options;
  options.insert(QString("color"), QVariant(color));
  options.insert(QString("scaleFactor"), QVariant(scaleFactor));

  std::vector<QString> iconNames;
  iconNames.emplace_back(iconName);

  std::vector<QHash<QString, QVariant>> optionsList;
  optionsList.emplace_back(options);
  return MantidQt::Icons::getIcon(iconNames, optionsList);
}

QIcon MantidQt::Icons::getIcon(
    const std::vector<QString> &iconNames,
    const std::vector<QHash<QString, QVariant>> &options) {
  auto &iconicFont = iconFontInstance();
  return iconicFont.getIcon(iconNames, options);
}

namespace MantidQt {
namespace Icons {

IconicFont::IconicFont() {
  this->loadFont(QString("mdi"), QString(":/mdi-font.ttf"),
                 QString(":/mdi-charmap.json"));
}

QIcon IconicFont::getIcon(
    const std::vector<QString> &iconNames,
    const std::vector<QHash<QString, QVariant>> &options) {

  std::vector<QHash<QString, QVariant>> actualOptions;
  // Assume there may be mutliple
  if (iconNames.size() != options.size()) {
    throw std::invalid_argument(
        "Icon names passed and options are not the same length");
  }
  actualOptions = options;
  for (auto i = 0u; i < iconNames.size(); ++i) {
    addValuesToOptions(actualOptions, iconNames, i);
  }

  return this->iconByPainter(&m_painter, actualOptions);
}

QIcon IconicFont::iconByPainter(
    CharIconPainter *painter, std::vector<QHash<QString, QVariant>> &options) {
  auto engine = new CharIconEngine(this, painter, options);
  return QIcon(engine);
}

void IconicFont::loadFont(const QString &prefix, const QString &ttfFilename,
                          const QString &charmapFilename) {
  const auto id = QFontDatabase::addApplicationFont(ttfFilename);
  const auto loadedFontFamilies = QFontDatabase::applicationFontFamilies(id);
  if (!loadedFontFamilies.empty()) {
    m_fontnames.insert(prefix, loadedFontFamilies[0]);
  }

  const auto hashValues = loadJsonFile(charmapFilename);
  m_charmap.insert(prefix, hashValues);
}

QFont IconicFont::getFont(const QString &prefix, const int drawSize) {
  QFont font(m_fontnames[prefix]);
  font.setPixelSize(drawSize);
  return font;
}

QHash<QString, QHash<QString, QVariant>> const IconicFont::getCharmap() const {
  return m_charmap;
}

} // namespace Icons
} // namespace MantidQt