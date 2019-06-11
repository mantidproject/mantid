// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PeakPicker.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

using namespace MantidQt::Widgets::MplCpp;

namespace {

QHash<QString, QVariant> defaultLineKwargs() {
  QHash<QString, QVariant> kwargs;
  kwargs.insert("line_style", QString("--"));
  return kwargs;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

PeakPicker::PeakPicker(PreviewPlot *plot, const QColor &colour)
    : QObject(), m_plot(plot), m_peakMarker(std::make_unique<PeakMarker>(
                                   m_plot->canvas(), 1, 2100.0, 0, 0.1, 20)) {
  m_plot->canvas()->draw();
  m_peakMarker->redraw();
}

} // namespace MantidWidgets
} // namespace MantidQt
