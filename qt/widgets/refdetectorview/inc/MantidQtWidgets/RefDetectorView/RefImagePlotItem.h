// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_IMAGE_PLOT_ITEM_H
#define REF_IMAGE_PLOT_ITEM_H

#include "DllOption.h"
#include "MantidQtWidgets/RefDetectorView/RefLimitsHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumPlotItem.h"

namespace MantidQt {
namespace RefDetectorViewer {
/** This class is responsible for actually drawing the image data onto
    a QwtPlot for the SpectrumView data viewer.
 */

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImagePlotItem
    : public SpectrumView::SpectrumPlotItem {

public:
  /// Construct basic plot item with NO data to plot.
  RefImagePlotItem(const RefLimitsHandler *const limitsHandler);

  ~RefImagePlotItem();

  /// Draw the image (this is called by QWT and must not be called directly.)
  virtual void draw(QPainter *painter, const QwtScaleMap &xMap,
                    const QwtScaleMap &yMap,
                    const QRect &canvasRect) const override;

private:
  const RefLimitsHandler *const m_limitsHandler;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_IMAGE_PLOT_ITEM_H
