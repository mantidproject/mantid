// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDPREFERENCES_H
#define MANTIDPREFERENCES_H

#include <QList>
#include <QMap>
#include <QString>

/** @class MantidPreferences

 Keeps the preferences for some Mantid-related UI settings.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 13/02/2009
 */

class MantidPreferences {
public:
  // ---  MantidMatrixSettings --- //

  static int MantidMatrixColumnWidthY();
  static int MantidMatrixColumnWidthX();
  static int MantidMatrixColumnWidthE();
  static int MantidMatrixColumnWidthDx();
  static void MantidMatrixColumnWidthY(int width);
  static void MantidMatrixColumnWidthX(int width);
  static void MantidMatrixColumnWidthE(int width);
  static void MantidMatrixColumnWidthDx(int width);
  static void MantidMatrixColumnWidth(int width);

  static QChar MantidMatrixNumberFormatY();
  static QChar MantidMatrixNumberFormatX();
  static QChar MantidMatrixNumberFormatE();
  static QChar MantidMatrixNumberFormatDx();
  static void MantidMatrixNumberFormatY(const QChar &f);
  static void MantidMatrixNumberFormatX(const QChar &f);
  static void MantidMatrixNumberFormatE(const QChar &f);
  static void MantidMatrixNumberFormatDx(const QChar &f);
  static void MantidMatrixNumberFormat(const QChar &f);

  static int MantidMatrixNumberPrecisionY();
  static int MantidMatrixNumberPrecisionX();
  static int MantidMatrixNumberPrecisionE();
  static int MantidMatrixNumberPrecisionDx();
  static void MantidMatrixNumberPrecisionY(int p);
  static void MantidMatrixNumberPrecisionX(int p);
  static void MantidMatrixNumberPrecisionE(int p);
  static void MantidMatrixNumberPrecisionDx(int p);
  static void MantidMatrixNumberPrecision(int p);

private:
  /// Creation of non-static instances is not allowed.
  MantidPreferences();
  MantidPreferences(const MantidPreferences &);
};

#endif /* MANTIDPREFERENCES_H */
