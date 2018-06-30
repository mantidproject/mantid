#ifndef MANTIDPLOT_MANTIDMATRIXNULLEXTENSIONHANDLER_H
#define MANTIDPLOT_MANTIDMATRIXNULLEXTENSIONHANDLER_H

#include "IMantidMatrixExtensionHandler.h"
#include "MantidKernel/Logger.h"
#include "MantidMatrixTabExtension.h"

namespace {
Mantid::Kernel::Logger g_log("MantidMatrixNullExtensionHandler");
}

class MantidMatrixNullExtensionHandler : public IMantidMatrixExtensionHandler {
public:
  MantidMatrixNullExtensionHandler() {}
  ~MantidMatrixNullExtensionHandler() override {}

  void setNumberFormat(MantidMatrixTabExtension &, const QChar &,
                       int) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  void recordFormat(MantidMatrixTabExtension &, const QChar &, int) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  QChar getFormat(MantidMatrixTabExtension &) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  int getPrecision(MantidMatrixTabExtension &) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  void setColumnWidth(MantidMatrixTabExtension &, int, int) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  int getColumnWidth(MantidMatrixTabExtension &) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  QTableView *getTableView(MantidMatrixTabExtension &) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  void setColumnWidthPreference(MantidMatrixTabExtension &, int) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }

  int getColumnWidthPreference(MantidMatrixTabExtension &) override {
    throw std::runtime_error(
        "You have seemed to attached an invalid extension to the "
        "Mantid Matrix.");
  }
};
#endif