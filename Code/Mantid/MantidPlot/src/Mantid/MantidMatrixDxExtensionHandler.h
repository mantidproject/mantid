#ifndef MANTIDPLOT_MANTIDMATRIXDXEXTENSIONHANDLER_H
#define MANTIDPLOT_MANTIDMATRIXDXEXTENSIONHANDLER_H

#include "IMantidMatrixExtensionHandler.h"
#include "MantidMatrixTabExtension.h"
#include "MantidMatrixModel.h"

class MantidMatrixDxExtensionHandler : public IMantidMatrixExtensionHandler {
public:
  MantidMatrixDxExtensionHandler();
  virtual ~MantidMatrixDxExtensionHandler();
  virtual void setNumberFormat(MantidMatrixTabExtension& extension, const QChar& format,int precision);
  virtual void recordFormat(MantidMatrixTabExtension& extension, const QChar &format, int precision);
  virtual QChar getFormat(MantidMatrixTabExtension& extension);
  virtual int getPrecision(MantidMatrixTabExtension& extension);
  virtual void setColumnWidth(MantidMatrixTabExtension& extension, int width, int numberOfColumns);
  virtual int getColumnWidth(MantidMatrixTabExtension& extension);
  virtual QTableView* getTableView(MantidMatrixTabExtension& extension);
  virtual void setColumnWidthPreference(MantidMatrixTabExtension& extension, int width);
  virtual int getColumnWidthPreference(MantidMatrixTabExtension& extension);
private:
  MantidMatrixModel::Type m_type;
};
#endif