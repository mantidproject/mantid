// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef QT_UTILS_H
#define QT_UTILS_H

#include <QLineEdit>
#include <QTableWidget>

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"

/**
    @class QtUtils

    This class has some static methods to simplify interaction with
    Qt and Qwt for the SpectrumView data viewer.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER QtUtils {
public:
  /// enter the specified string in the table
  static void SetTableEntry(int row, int col, const std::string &string,
                            QTableWidget *table);

  /// enter the specified double, formatted, in the table
  static void SetTableEntry(int row, int col, int width, int precision,
                            double value, QTableWidget *table);

  /// Set the specified string into the specified QLineEdit widget.
  static void SetText(const std::string &string, QLineEdit *lineEdit);

  /// enter the specified double, formatted, in the QLineEdit control
  static void SetText(int width, int precision, double value,
                      QLineEdit *lineEdit);
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // QT_UTILS_H
