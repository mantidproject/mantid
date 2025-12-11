// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <stdexcept>

namespace {

void writeData(const QString &filename, const QByteArray &data) {
  QFile jsonFile(filename);

  if (!jsonFile.open(QFile::WriteOnly)) {
    throw std::invalid_argument("Could not open file at " + filename.toStdString());
  }

  if (jsonFile.write(data) == -1) {
    throw std::runtime_error("Failed to write data to " + filename.toStdString());
  }
}

} // namespace
namespace MantidQt::API {
void saveJSONToFile(QString &filename, const QMap<QString, QVariant> &map) {
  auto filenameString = filename.toStdString();
  if (filenameString.find_last_of(".") == std::string::npos ||
      filenameString.substr(filenameString.find_last_of(".") + 1) != std::string("json")) {
    filename += ".json";
  }

  QJsonDocument jsonDocument(QJsonObject::fromVariantMap(map));
  writeData(filename, jsonDocument.toJson());
}

QMap<QString, QVariant> loadJSONFromFile(const QString &filename) {
  /* https://stackoverflow.com/questions/19822211/qt-parsing-json-using-qjsondocument-qjsonobject-qjsonarray
   * is the source for a large portion of the source code for the Qt5
   * implementation of this function, from user alanwsx and edited by BSMP.
   */
  QFile file(filename);
  if (!file.open(QFile::ReadOnly)) {
    throw std::invalid_argument("Cannot open file at: " + filename.toStdString());
  }

  // step 2
  QTextStream text(&file);
  QString jsonString;
  jsonString = text.readAll();
  file.close();

  return loadJSONFromString(jsonString);
}

QMap<QString, QVariant> loadJSONFromString(const QString &jsonString) {
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
}

std::string outputJsonToString(const QMap<QString, QVariant> &map) {
  (void)map;
  return {};
}

} // namespace MantidQt::API
