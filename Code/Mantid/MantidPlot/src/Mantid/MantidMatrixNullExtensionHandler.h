#ifndef MANTIDPLOT_MANTIDMATRIXNULLEXTENSIONHANDLER_H
#define MANTIDPLOT_MANTIDMATRIXNULLEXTENSIONHANDLER_H

#include "IMantidMatrixExtensionHandler.h"
#include "MantidMatrixTabExtension.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("MantidMatrixNullExtensionHandler");
}

class MantidMatrixNullExtensionHandler : public IMantidMatrixExtensionHandler {
public:
  MantidMatrixNullExtensionHandler() {}
  virtual ~MantidMatrixNullExtensionHandler() {}

  virtual void
  setSuccessor(boost::shared_ptr<IMantidMatrixExtensionHandler> successor) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void setNumberFormat(MantidMatrixTabExtension& extension,
                               const QChar &format, int precision) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void recordFormat(MantidMatrixTabExtension& extension, const QChar &format, int precision) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual QChar getFormat(MantidMatrixTabExtension& extension) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getPrecision(MantidMatrixTabExtension& extension) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void setColumnWidth(MantidMatrixTabExtension& extension, int width, int numberOfColumns) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getColumnWidth(MantidMatrixTabExtension& extension) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual QTableView* getTableView(MantidMatrixTabExtension& extension) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void setColumnWidthPreference(MantidMatrixTabExtension& extension, int width) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getColumnWidthPreference(MantidMatrixTabExtension& extension) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }
};
#endif