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
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
namespace {
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
    engine.evaluate("function toString() { return JSON.stringify(this) }");

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
} // namespace
#endif
namespace MantidQt {
namespace API {
void saveJSONToFile(const QString &filename,
                    const QMap<QString, QVariant> &map) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  JSON JSON;
  auto jsonString = JSON.encode(map);
  QFile jsonFile(filename);
  jsonFile.open(QFile::WriteOnly);
  QByteArray jsonByteArray;
  jsonFile.write(jsonByteArray.append(jsonString));
#else
  QJsonDocument jsonDocument(QJsonObject::fromVariantMap(map));
  QFile jsonFile(filename);
  jsonFile.open(QFile::WriteOnly);
  jsonFile.write(jsonDocument.toJson());
#endif
}

QMap<QString, QVariant> loadJSONFromFile(const QString &filename) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  JSON JSON;
  QFile jsonFile(filename);
  jsonFile.open(QFile::ReadOnly);
  QString json(jsonFile.readAll());
  return JSON.decode(json);
#else
  QFile jsonFile(filename);
  jsonFile.open(QFile::ReadOnly);
  QJsonDocument jsonDocument;
  jsonDocument.fromJson(jsonFile.readAll());
  return jsonDocument.object().toVariantMap();

#endif
}
} // namespace API
} // namespace MantidQt