// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

namespace MantidQt {
namespace MantidWidgets {

PreviewPlot::PreviewPlot(QWidget *parent, bool init) : QWidget(parent) {}

void PreviewPlot::addSpectrum(const QString &curveName,
                              const Mantid::API::MatrixWorkspace_sptr &ws,
                              const size_t wsIndex = 0,
                              const QColor &curveColour = QColor()) {
  throw std::runtime_error("addSpectrum1 unimplemented");
}

void PreviewPlot::addSpectrum(const QString &curveName, const QString &wsName,
                              const size_t wsIndex = 0,
                              const QColor &curveColour = QColor()) {
  throw std::runtime_error("addSpectrum2 unimplemented");
}

void PreviewPlot::removeSpectrum(const QString &curveName) {
  throw std::runtime_error("removeSpectrum unimplemented");
}

void PreviewPlot::setAxisRange(QPair<double, double> range, int axisID) {
  throw std::runtime_error("setAxisRange unimplemented");
}

void PreviewPlot::clear() { throw std::runtime_error("clear unimplemented"); }

void PreviewPlot::resizeX() {
  throw std::runtime_error("resizeX unimplemented");
}

} // namespace MantidWidgets
} // namespace MantidQt
