// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidQtWidgets/MplCpp/PeakMarker.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays a peak picker tool for selecting a peak on a previewplot
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PeakPicker : public QObject {
  Q_OBJECT

public:
  PeakPicker(PreviewPlot *plot);

  void redraw();
  void remove();

  void setPeak(const Mantid::API::IPeakFunction_const_sptr &peak, const double background = 0.0);
  Mantid::API::IPeakFunction_sptr peak() const;

  void select(bool select);
  void setVisible(bool visible);

signals:
  void changed();

  // cppcheck-suppress unknownMacro
private slots:
  void handleMouseDown(const QPoint &point);
  void handleMouseMove(const QPoint &point);
  void handleMouseUp(const QPoint &point);
  void handleMouseHovering(const QPoint &point);

  void redrawMarker();

private:
  /// The preview plot containing the range selector
  PreviewPlot *m_plot;
  /// Currently represented peak
  Mantid::API::IPeakFunction_sptr m_peak;
  /// The minimum marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::PeakMarker> m_peakMarker;
};

} // namespace MantidWidgets
} // namespace MantidQt
