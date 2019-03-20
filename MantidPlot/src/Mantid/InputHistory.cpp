// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InputHistory.h"
#include <MantidAPI/Algorithm.h>

#include <QSettings>
#include <QStringList>
#include <iostream>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;

/// Constructor
InputHistoryImpl::InputHistoryImpl() {
  QSettings settings;
  settings.beginGroup("Mantid/Algorithms");

  QStringList keys = settings.allKeys();
  QStringList algNames;
  QString prevName = "";
  for (int i = 0; i < keys.size(); i++) {
    if (keys[i] != prevName) {
      prevName = keys[i].split("/")[0];
      algNames << prevName;
    }
  }
  for (int i = 0; i < algNames.size(); i++) {
    QString algName = algNames[i];
    QList<PropertyData> prop_hist_list;
    settings.beginGroup(algName);
    QStringList keys = settings.allKeys();
    for (int i = 0; i < keys.size(); i++) {
      QString value = settings.value(keys[i]).toString();
      PropertyData prop_hist(keys[i], value);
      prop_hist_list.push_back(prop_hist);
    }
    settings.endGroup();
    m_history[algName] = prop_hist_list;
  }
  settings.endGroup();
}

void InputHistoryImpl::save() {
  QSettings settings;
  settings.beginGroup("Mantid/Algorithms");
  QMapIterator<QString, QList<PropertyData>> alg(m_history);
  while (alg.hasNext()) {
    alg.next();
    const QList<PropertyData> &prop_hist = alg.value();
    settings.beginGroup(alg.key());
    for (const auto &prop : prop_hist)
      settings.setValue(prop.name, prop.value);
    settings.endGroup();
  }
  settings.endGroup();
}
/**
     Upadates the non-default algorithm properties in the history.
     @param alg :: Pointer to the algorthm
*/
void InputHistoryImpl::updateAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  const std::vector<Property *> &props = alg->getProperties();
  QList<PropertyData> prop_hist_list;
  for (auto &prop : props)
    if (!prop->isDefault()) {
      PropertyData prop_hist(QString::fromStdString(prop->name()),
                             QString::fromStdString(prop->value()));
      prop_hist_list.push_back(prop_hist);
    } else {
      PropertyData prop_hist(QString::fromStdString(prop->name()), "");
      prop_hist_list.push_back(prop_hist);
    }
  m_history[QString::fromStdString(alg->name())] = prop_hist_list;
}

void InputHistoryImpl::printAll() {
  QMapIterator<QString, QList<PropertyData>> alg(m_history);
  while (alg.hasNext()) {
    alg.next();
    std::cerr << alg.key().toStdString() << '\n';
    const QList<PropertyData> &prop_list = alg.value();
    for (const auto &prop : prop_list)
      std::cerr << prop.name.toStdString() << ": " << prop.value.toStdString()
                << '\n';
  }
}

/**
    @param algName :: Algorithm name
*/
QMap<QString, QString>
InputHistoryImpl::algorithmProperties(const QString &algName) {
  QMap<QString, QList<PropertyData>>::const_iterator a =
      m_history.find(algName);
  if (a != m_history.end()) {
    QMap<QString, QString> m;
    const QList<PropertyData> &prop_list = a.value();
    for (const auto &prop : prop_list)
      m[prop.name] = prop.value;
    return m;
  }
  return QMap<QString, QString>();
}

/**  Returns the last entered value for property propName
     @param algName :: Name of the algorithm
     @param propName :: Property
     @return The last entered value for the property or the empty string if the
   default value was used.
*/

QString InputHistoryImpl::algorithmProperty(const QString &algName,
                                            const QString &propName) {
  QMap<QString, QString> prop = algorithmProperties(algName);
  return prop[propName];
}

QString InputHistoryImpl::getDirectoryFromFilePath(const QString &filePath) {
  QString s = filePath;
  int i = s.lastIndexOf('\\');
  if (i < 0)
    i = s.lastIndexOf('/');
  if (i < 0)
    i = 0;
  return s.remove(i, s.length() - i);
}

QString InputHistoryImpl::getNameOnlyFromFilePath(const QString &filePath) {
  const QString &s = filePath;
  int i = s.lastIndexOf('\\');
  if (i < 0)
    i = s.lastIndexOf('/');
  if (i < 0)
    return s;
  int j = s.lastIndexOf('.');
  if (j < 0)
    j = s.length();
  return s.mid(i + 1, j - i - 1);
}

void InputHistoryImpl::updateAlgorithmProperty(const QString &algName,
                                               const QString &propName,
                                               const QString &propValue) {
  QMap<QString, QList<PropertyData>>::iterator a = m_history.find(algName);
  if (a == m_history.end()) {
    QList<PropertyData> newList;
    newList << PropertyData(propName, propValue);
    m_history[algName] = newList;
  } else {
    QList<PropertyData> &propList = a.value();
    propList << PropertyData(propName, propValue);
  }
}
