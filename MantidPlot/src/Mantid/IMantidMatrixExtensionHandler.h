#ifndef MANTIDPLOT_IMANTIDMATRIXEXTENSIONHANDLER_H
#define MANTIDPLOT_IMANTIDMATRIXEXTENSIONHANDLER_H

#include "MantidMatrixTabExtension.h"
#include "MantidKernel/Chainable.h"
#include "boost/shared_ptr.hpp"

class IMantidMatrixExtensionHandler : public Mantid::Kernel::Chainable<IMantidMatrixExtensionHandler> {
public:
  virtual ~IMantidMatrixExtensionHandler() {}
  virtual void setNumberFormat(MantidMatrixTabExtension& extension,
                               const QChar &format, int precision) = 0;
  virtual void recordFormat(MantidMatrixTabExtension& extension, const QChar &format, int precision) = 0;
  virtual QChar getFormat(MantidMatrixTabExtension& extension) = 0;
  virtual int getPrecision(MantidMatrixTabExtension& extension) = 0;
  virtual void setColumnWidth(MantidMatrixTabExtension& extension, int width, int numberOfColumns) = 0;
  virtual int getColumnWidth(MantidMatrixTabExtension& extension) = 0;
  virtual QTableView* getTableView(MantidMatrixTabExtension& extension) = 0;
  virtual void setColumnWidthPreference(MantidMatrixTabExtension& extension, int width)  = 0;
  virtual int getColumnWidthPreference(MantidMatrixTabExtension& extension) = 0;
};
#endif
