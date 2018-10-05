// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_IMAGE_DISPLAY_H
#define REF_IMAGE_DISPLAY_H

#include "DllOption.h"
#include "MantidQtWidgets/RefDetectorView/RefLimitsHandler.h"
#include "MantidQtWidgets/RefDetectorView/RefRangeHandler.h"
#include "MantidQtWidgets/RefDetectorView/RefSliderHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h"

namespace MantidQt {
namespace RefDetectorViewer {

/** This class extends the SpectrumViewer::SpectrumDisplay class to communicate
   left-clicks
    to the RefLimitsHandler (and thence to the peak/background/TOF line edits in
   the gui)
 */

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageDisplay
    : public SpectrumView::SpectrumDisplay {
public:
  /// Make a SpectrumDisplay to display with the given widgets and controls
  RefImageDisplay(QwtPlot *imagePlot, RefSliderHandler *sliderHandler,
                  RefRangeHandler *rangeHandler,
                  RefLimitsHandler *limitsHandler,
                  SpectrumView::GraphDisplay *hGraph,
                  SpectrumView::GraphDisplay *vGraph,
                  QTableWidget *tableWidget);

  ~RefImageDisplay() override;

  /// Record the point that the user is currently pointing at with the mouse
  /// default right click (mouseClick = 2)
  QPair<double, double> setPointedAtPoint(QPoint point, int mouseClick = 2,
                                          bool isFirst = true) override;

private:
  RefLimitsHandler *m_limitsHandler; // Owned by RefImagePlotItem
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_IMAGE_DISPLAY_H
