// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_IMAGE_VIEW_H
#define REF_IMAGE_VIEW_H

#include <QMainWindow>

#include "DllOption.h"
#include "MantidQtWidgets/SpectrumViewer/GraphDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"

/**
    @class RefImageView

    This is the QMainWindow for the SpectrumView data viewer.  Data is
    displayed in an SpectrumView, by constructing the SpectrumView object and
    specifying a particular data source.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace Ui {
class RefImageViewer;
}

namespace MantidQt {
namespace RefDetectorViewer {
class RefSliderHandler;
class RefRangeHandler;
class RefImageDisplay;
class RefIVConnections;

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageView : public QMainWindow {
public:
  /// Construct an RefImageView to display data from the specified data source
  RefImageView(SpectrumView::SpectrumDataSource_sptr dataSource, int peakMin,
               int peakMax, int backMin, int backMax, int tofMin, int tofMax);

  ~RefImageView() override;

  RefIVConnections *getIVConnections();

private:
  SpectrumView::GraphDisplay *m_hGraph;
  SpectrumView::GraphDisplay *m_vGraph;

  Ui::RefImageViewer *m_ui;
  RefSliderHandler *m_sliderHandler;
  RefRangeHandler *m_rangeHandler;
  RefImageDisplay *m_imageDisplay;
  RefIVConnections *m_ivConnections;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_IMAGE_VIEW_H
