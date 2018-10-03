// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Preferences.h"
#include <QSettings>

//------------------------------------------//
//           MantidMatrix settings
//------------------------------------------//

int MantidPreferences::MantidMatrixColumnWidthY() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/ColumnWidthY", 100)
      .toInt();
}

int MantidPreferences::MantidMatrixColumnWidthX() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/ColumnWidthX", 100)
      .toInt();
}

int MantidPreferences::MantidMatrixColumnWidthE() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/ColumnWidthE", 100)
      .toInt();
}

int MantidPreferences::MantidMatrixColumnWidthDx() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/ColumnWidthDx", 100)
      .toInt();
}

void MantidPreferences::MantidMatrixColumnWidthY(int width) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthY", width);
}

void MantidPreferences::MantidMatrixColumnWidthX(int width) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthX", width);
}

void MantidPreferences::MantidMatrixColumnWidthE(int width) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthE", width);
}

void MantidPreferences::MantidMatrixColumnWidthDx(int width) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthDx", width);
}

void MantidPreferences::MantidMatrixColumnWidth(int width) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthY", width);
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthX", width);
  settings.setValue("Mantid/Preferences/MantidMatrix/ColumnWidthE", width);
}

QChar MantidPreferences::MantidMatrixNumberFormatY() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberFormatY", 'g')
      .toChar();
}

QChar MantidPreferences::MantidMatrixNumberFormatX() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberFormatX", 'g')
      .toChar();
}

QChar MantidPreferences::MantidMatrixNumberFormatE() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberFormatE", 'g')
      .toChar();
}

QChar MantidPreferences::MantidMatrixNumberFormatDx() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberFormatDx", 'g')
      .toChar();
}

void MantidPreferences::MantidMatrixNumberFormatY(const QChar &f) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatY", f);
}

void MantidPreferences::MantidMatrixNumberFormatX(const QChar &f) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatX", f);
}

void MantidPreferences::MantidMatrixNumberFormatE(const QChar &f) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatE", f);
}

void MantidPreferences::MantidMatrixNumberFormatDx(const QChar &f) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatDx", f);
}

void MantidPreferences::MantidMatrixNumberFormat(const QChar &f) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatY", f);
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatX", f);
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberFormatE", f);
}

int MantidPreferences::MantidMatrixNumberPrecisionY() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberPrecisionY", 6)
      .toInt();
}

int MantidPreferences::MantidMatrixNumberPrecisionX() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberPrecisionX", 6)
      .toInt();
}

int MantidPreferences::MantidMatrixNumberPrecisionE() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberPrecisionE", 6)
      .toInt();
}

int MantidPreferences::MantidMatrixNumberPrecisionDx() {
  QSettings settings;
  return settings.value("Mantid/Preferences/MantidMatrix/NumberPrecisionDx", 6)
      .toInt();
}

void MantidPreferences::MantidMatrixNumberPrecisionY(int p) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionY", p);
}

void MantidPreferences::MantidMatrixNumberPrecisionX(int p) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionX", p);
}

void MantidPreferences::MantidMatrixNumberPrecisionE(int p) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionE", p);
}

void MantidPreferences::MantidMatrixNumberPrecisionDx(int p) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionDx", p);
}

void MantidPreferences::MantidMatrixNumberPrecision(int p) {
  QSettings settings;
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionY", p);
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionX", p);
  settings.setValue("Mantid/Preferences/MantidMatrix/NumberPrecisionE", p);
}
