#ifndef MANTIDPLOT_IMANTIDMATRIXEXTENSION_H
#define MANTIDPLOT_IMANTIDMATRIXEXTENSION_H

#include "MantidMatrix.h"
#include <QObject>

class IMantidMatrixExtension {
  /// Create a MantidMatrixTabExtension
  void createMantidMatrixTabExtension(MantidMatrixTabExtensionCollection& collection) = 0;

  /// Check if it is the table view
  void isTableView(MantidMatrixTabExtensionCollection& collection, QObject *object) = 0

  /// Set the columns width
  void setColumnsWidth(MantidMatrixTabExtensionCollection& collection, int width, bool all) = 0;

  /// Set number format
  void setNumberFormat(MantidMatrixTabExtensionCollection& collection, const QChar& f,int prec)) = 0;



}
#endif