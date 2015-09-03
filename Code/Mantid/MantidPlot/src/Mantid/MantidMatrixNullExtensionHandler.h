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

  virtual void setNumberFormat(MantidMatrixTabExtension& ,
                               const QChar &, int ) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void recordFormat(MantidMatrixTabExtension& , const QChar&, int) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual QChar getFormat(MantidMatrixTabExtension&) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getPrecision(MantidMatrixTabExtension&) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void setColumnWidth(MantidMatrixTabExtension&, int, int) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getColumnWidth(MantidMatrixTabExtension&) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual QTableView* getTableView(MantidMatrixTabExtension&) {
    throw  std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual void setColumnWidthPreference(MantidMatrixTabExtension&, int) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }

  virtual int getColumnWidthPreference(MantidMatrixTabExtension&) {
    throw std::runtime_error("You have seemed to attached an invalid extension to the "
                "Mantid Matrix.");
  }
};
#endif