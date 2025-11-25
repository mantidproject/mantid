// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtIcons/Icon.h"

#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>

#include <stdexcept>

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

} // namespace

QIcon MantidQt::Icons::getIcon(const QString &iconName, const QString &color, const double &scaleFactor) {
  QHash<QString, QVariant> options;
  options.insert(QString("color"), QVariant(color));
  options.insert(QString("scaleFactor"), QVariant(scaleFactor));

  QStringList iconNames;
  iconNames.append(iconName);

  QList<QHash<QString, QVariant>> optionsList;
  optionsList.append(options);
  return MantidQt::Icons::getIcon(iconNames, optionsList);
}

QIcon MantidQt::Icons::getIcon(const QStringList &iconNames, const QList<QVariant> &options) {
  QList<QHash<QString, QVariant>> newOptions;
  for (auto option : options) {
    newOptions.append(option.toHash());
  }
  return MantidQt::Icons::getIcon(iconNames, newOptions);
}

QIcon MantidQt::Icons::getIcon(const QStringList &iconNames, const QList<QHash<QString, QVariant>> &options) {
  auto &iconicFont = iconFontInstance();
  return iconicFont.getIcon(iconNames, options);
}

namespace MantidQt::Icons {

IconicFont::IconicFont() : m_fontnames(), m_charmap(), m_painter() {
  this->loadFont(QString("mdi"), QString(":/mdi-font.ttf"), QString(":/mdi-charmap.json"));
}

QIcon IconicFont::getIcon(const QStringList &iconNames, const QList<QHash<QString, QVariant>> &options) {

  QList<QHash<QString, QVariant>> actualOptions;
  // Assume there may be mutliple
  if (iconNames.size() != options.size()) {
    throw std::invalid_argument("Icon names passed and options are not the same length");
  }
  actualOptions = options;
  for (auto i = 0; i < iconNames.size(); ++i) {
    addValuesToOptions(actualOptions, iconNames, i);
  }

  return this->iconByPainter(&m_painter, actualOptions);
}

QIcon IconicFont::iconByPainter(CharIconPainter *painter, QList<QHash<QString, QVariant>> const &options) {
  auto engine = new CharIconEngine(this, painter, options);
  return QIcon(engine);
}

void IconicFont::loadFont(const QString &prefix, const QString &ttfFilename, const QString &charmapFilename) {
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

QString IconicFont::findCharacterFromCharMap(const QString &prefix, const QString &character) const {
  return m_charmap[prefix][character].toString();
}

void IconicFont::addValuesToOptions(QList<QHash<QString, QVariant>> &options, const QStringList &iconNames,
                                    unsigned int vectorIndex) {
  const auto iconName = iconNames[vectorIndex];
  auto splitValues = iconName.split('.', Qt::SkipEmptyParts);
  const auto prefix = splitValues.at(0);
  const auto character = splitValues.at(1);

  const auto foundFontForPrefix = m_fontnames[prefix];
  if (foundFontForPrefix.isNull()) {
    throw std::invalid_argument(
        ("The prefix: \"" + prefix + "\" does not represent a set of icons currently availible").toStdString());
  }

  if (findCharacterFromCharMap(prefix, character).isNull()) {
    throw std::invalid_argument(
        ("The icon: \"" + prefix + "." + character + "\" is not a icon currently availible in the library")
            .toStdString());
  }

  options[vectorIndex].insert(QString("prefix"), QVariant(prefix));
  options[vectorIndex].insert(QString("character"), QVariant(character));
}

} // namespace MantidQt::Icons
