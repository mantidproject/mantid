#ifndef MANTIDPLOT_MANTIDMATRIXDXEXTENSIONHANDLER_H
#define MANTIDPLOT_MANTIDMATRIXDXEXTENSIONHANDLER_H

#include "IMantidMatrixExtensionHandler.h"
#include "MantidMatrixTabExtension.h"
#include "MantidMatrixModel.h"

class MantidMatrixDxExtensionHandler : public IMantidMatrixExtensionHandler {
public:
  MantidMatrixDxExtensionHandler();
  virtual ~MantidMatrixDxExtensionHandler();
  virtual void setSuccessor(boost::shared_ptr<IMantidMatrixExtensionHandler> successor);
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
  boost::shared_ptr<IMantidMatrixExtensionHandler> m_successor;
  MantidMatrixModel::Type m_type;
};
#endif