// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtIcons/Icon.h"

#include <QFile>
#include <QFontDatabase>
#include <boost/algorithm/string.hpp>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValueIterator>
#else
#include <QJsonDocument>
#include <QJsonObject>
#endif

namespace {
MantidQt::Icons::IconicFont &iconFontInstance() {
  static MantidQt::Icons::IconicFont iconicFont;
  return iconicFont;
}
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
// In Qt4 there is no access to QJsonDocument and a method for reading JSON in
// the QT standard library so the Qt4 method must be used.

// This code for decode and decodeInner was inspired and
// taken from:
// https://stackoverflow.com/questions/4169988/easiest-way-to-parse-json-in-qt-4-7
// specifically in the answer by user2243820 on April 4th 2013 at 8:10

QHash<QString, QVariant> decodeInner(QScriptValue object) {
  QHash<QString, QVariant> map;
  QScriptValueIterator it(object);
  while (it.hasNext()) {
    it.next();
    if (it.value().isString())
      map.insert(it.name(), QVariant(it.value().toString()));
    else
      throw std::runtime_error(
          "Decoding JSON file not successful as some values are not strings.");
  }
  return map;
}

QHash<QString, QVariant> decode(const QString &jsonStr) {
  QScriptValue object;
  QScriptEngine engine;
  object = engine.evaluate("(" + jsonStr + ")");
  return decodeInner(object);
}

QHash<QString, QVariant> loadJsonFile(const QString &charmapFileName) {
  QFile jsonFile(charmapFileName);
  jsonFile.open(QFile::ReadOnly);
  return decode(jsonFile.readAll());
}

#else
QHash<QString, QVariant> loadJsonFile(const QString &charmapFileName) {
  QFile jsonFile(charmapFileName);
  jsonFile.open(QFile::ReadOnly);
  const auto jsonDocument = QJsonDocument().fromJson(jsonFile.readAll());
  const auto jsonObject = jsonDocument.object();
  // VariantHash = QHash<QString, QVariant> in this case QVaraint = QString
  return jsonObject.toVariantHash();
}
#endif
} // namespace

QIcon MantidQt::Icons::getIcon(const QString &iconName, const QString &color,
                               const double &scaleFactor) {
  QHash<QString, QVariant> options;
  options.insert(QString("color"), QVariant(color));
  options.insert(QString("scaleFactor"), QVariant(scaleFactor));

  QStringList iconNames;
  iconNames.append(iconName);

  QList<QHash<QString, QVariant>> optionsList;
  optionsList.append(options);
  return MantidQt::Icons::getIcon(iconNames, optionsList);
}

QIcon MantidQt::Icons::getIcon(const QStringList &iconNames,
                               const QList<QVariant> &options) {
  QList<QHash<QString, QVariant>> newOptions;
  for (auto option : options) {
    newOptions.append(option.toHash());
  }
  return MantidQt::Icons::getIcon(iconNames, newOptions);
}

QIcon MantidQt::Icons::getIcon(const QStringList &iconNames,
                               const QList<QHash<QString, QVariant>> &options) {
  auto &iconicFont = iconFontInstance();
  return iconicFont.getIcon(iconNames, options);
}

namespace MantidQt {
namespace Icons {

IconicFont::IconicFont() : m_fontnames(), m_charmap(), m_painter() {
  this->loadFont(QString("mdi"), QString(":/mdi-font.ttf"),
                 QString(":/mdi-charmap.json"));
}

QIcon IconicFont::getIcon(const QStringList &iconNames,
                          const QList<QHash<QString, QVariant>> &options) {

  QList<QHash<QString, QVariant>> actualOptions;
  // Assume there may be mutliple
  if (iconNames.size() != options.size()) {
    throw std::invalid_argument(
        "Icon names passed and options are not the same length");
  }
  actualOptions = options;
  for (auto i = 0; i < iconNames.size(); ++i) {
    addValuesToOptions(actualOptions, iconNames, i);
  }

  return this->iconByPainter(&m_painter, actualOptions);
}

QIcon IconicFont::iconByPainter(CharIconPainter *painter,
                                QList<QHash<QString, QVariant>> &options) {
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

QString IconicFont::findCharecterFromCharMap(const QString &prefix,
                                             const QString &charecter) const {
  return m_charmap[prefix][charecter].toString();
}

void IconicFont::addValuesToOptions(QList<QHash<QString, QVariant>> &options,
                                    const QStringList iconNames,
                                    unsigned int vectorIndex) {
  const auto iconName = iconNames[vectorIndex];
  auto splitValues = iconName.split('.', QString::SkipEmptyParts);
  const auto prefix = splitValues.at(0);
  const auto charecter = splitValues.at(1);

  const auto foundFontForPrefix = m_fontnames[prefix];
  if (foundFontForPrefix.isNull()) {
    throw std::invalid_argument(
        ("The prefix: \"" + prefix +
         "\" does not represent a set of icons currently availible")
            .toStdString());
  }

  if (findCharecterFromCharMap(prefix, charecter).isNull()) {
    throw std::invalid_argument(
        ("The icon: \"" + prefix + "." + charecter +
         "\" is not a icon currently availible in the library")
            .toStdString());
  }

  options[vectorIndex].insert(QString("prefix"), QVariant(prefix));
  options[vectorIndex].insert(QString("charecter"), QVariant(charecter));
}

} // namespace Icons
} // namespace MantidQt