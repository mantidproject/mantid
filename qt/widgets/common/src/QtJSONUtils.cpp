// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <QFile>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>
#else
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#endif

namespace {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// Code in anonymous namespace taken from stack overflow
// https://stackoverflow.com/questions/4169988/easiest-way-to-parse-json-in-qt-4-7
// from user2243820 on 1st August 2019

class JSON {
public:
  QScriptValue encodeInner(const QMap<QString, QVariant> &map,
                           QScriptEngine *engine) {
    QScriptValue obj = engine->newObject();
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
      i.next();
      if (i.value().type() == QVariant::String)
        obj.setProperty(i.key(), i.value().toString());
      else if (i.value().type() == QVariant::Int)
        obj.setProperty(i.key(), i.value().toInt());
      else if (i.value().type() == QVariant::Double)
        obj.setProperty(i.key(), i.value().toDouble());
      else if (i.value().type() == QVariant::Bool)
        obj.setProperty(i.key(), i.value().toBool());
      else if (i.value().type() == QVariant::List)
        obj.setProperty(i.key(),
                        qScriptValueFromSequence(engine, i.value().toList()));
      else if (i.value().type() == QVariant::Map)
        obj.setProperty(i.key(), encodeInner(i.value().toMap(), engine));
    }
    return obj;
  }

  QString encode(const QMap<QString, QVariant> &map) {
    QScriptEngine engine;
    engine.evaluate(
        "function toString() { return JSON.stringify(this, null, 4) }");

    QScriptValue toString = engine.globalObject().property("toString");
    QScriptValue obj = encodeInner(map, &engine);
    return toString.call(obj).toString();
  }

  QMap<QString, QVariant> decodeInner(QScriptValue object) {
    QMap<QString, QVariant> map;
    QScriptValueIterator it(object);
    while (it.hasNext()) {
      it.next();
      if (it.value().isArray())
        map.insert(it.name(), QVariant(decodeInnerToList(it.value())));
      else if (it.value().isNumber())
        map.insert(it.name(), QVariant(it.value().toNumber()));
      else if (it.value().isString())
        map.insert(it.name(), QVariant(it.value().toString()));
      else if (it.value().isNull())
        map.insert(it.name(), QVariant());
      else if (it.value().isObject())
        map.insert(it.name(), QVariant(decodeInner(it.value())));
      else if (it.value().isBool())
        map.insert(it.name(), QVariant(it.value().toBool()));
    }
    return map;
  }

  QList<QVariant> decodeInnerToList(QScriptValue arrayValue) {
    QList<QVariant> list;
    QScriptValueIterator it(arrayValue);
    while (it.hasNext()) {
      it.next();
      if (it.name() == "length")
        continue;

      if (it.value().isArray())
        list.append(QVariant(decodeInnerToList(it.value())));
      else if (it.value().isNumber())
        list.append(QVariant(it.value().toNumber()));
      else if (it.value().isString())
        list.append(QVariant(it.value().toString()));
      else if (it.value().isBool())
        list.append(QVariant(it.value().toBool()));
      else if (it.value().isNull())
        list.append(QVariant());
      else if (it.value().isObject())
        list.append(QVariant(decodeInner(it.value())));
    }
    return list;
  }

  QMap<QString, QVariant> decode(const QString &jsonStr) {
    QScriptValue object;
    QScriptEngine engine;
    object = engine.evaluate("(" + jsonStr + ")");
    return decodeInner(object);
  }
};
#endif

void writeData(const QString &filename, const QByteArray &data) {
  QFile jsonFile(filename);

  if (!jsonFile.open(QFile::WriteOnly)) {
    throw std::invalid_argument("Could not open file at " +
                                filename.toStdString());
  }

  if (jsonFile.write(data) == -1) {
    throw std::runtime_error("Failed to write data to " +
                             filename.toStdString());
  }
}

} // namespace
namespace MantidQt {
namespace API {
void saveJSONToFile(QString &filename, const QMap<QString, QVariant> &map) {
  auto filenameString = filename.toStdString();
  if (filenameString.find_last_of(".") == std::string::npos ||
      filenameString.substr(filenameString.find_last_of(".") + 1) !=
          std::string("json")) {
    filename += ".json";
  }
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  JSON JSON;
  auto jsonString = JSON.encode(map);

  QByteArray jsonByteArray;
  writeData(filename, jsonByteArray.append(jsonString));
#else
  QJsonDocument jsonDocument(QJsonObject::fromVariantMap(map));
  writeData(filename, jsonDocument.toJson());
#endif
}

QMap<QString, QVariant> loadJSONFromFile(const QString &filename) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  QFile jsonFile(filename);
  if (!jsonFile.open(QFile::ReadOnly)) {
    throw std::invalid_argument("Cannot open file at: " +
                                filename.toStdString());
  }
  QString jsonString(jsonFile.readAll());
  return loadJSONFromString(jsonString);
#else
  /* https://stackoverflow.com/questions/19822211/qt-parsing-json-using-qjsondocument-qjsonobject-qjsonarray
   * is the source for a large portion of the source code for the Qt5
   * implementation of this function, from user alanwsx and edited by BSMP.
   */
  QFile file(filename);
  if (!file.open(QFile::ReadOnly)) {
    throw std::invalid_argument("Cannot open file at: " +
                                filename.toStdString());
  }

  // step 2
  QTextStream text(&file);
  QString jsonString;
  jsonString = text.readAll();
  file.close();

  return loadJSONFromString(jsonString);

#endif
}

QMap<QString, QVariant> loadJSONFromString(const QString &jsonString) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  JSON JSON;
  return JSON.decode(jsonString);
#else
  QByteArray jsonByteArray = jsonString.toLocal8Bit();

  auto jsonDoc = QJsonDocument::fromJson(jsonByteArray);
  if (jsonDoc.isNull()) {
    throw std::runtime_error("Failed to create JSON doc.");
  }
  if (!jsonDoc.isObject()) {
    throw std::runtime_error("JSON is not an object.");
  }

  QJsonObject jsonObj = jsonDoc.object();
  if (jsonObj.isEmpty()) {
    throw std::runtime_error("JSON object is empty.");
  }

  return jsonObj.toVariantMap();
#endif
}

} // namespace API
} // namespace MantidQt
