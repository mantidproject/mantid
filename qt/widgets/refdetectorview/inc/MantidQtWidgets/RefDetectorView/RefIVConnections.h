// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_IV_CONNECTIONS_H
#define REF_IV_CONNECTIONS_H

#include <QActionGroup>
#include <QWidget>

#include "DllOption.h"
#include "MantidQtWidgets/RefDetectorView/RefImageDisplay.h"
#include "MantidQtWidgets/RefDetectorView/RefImageView.h"
#include "MantidQtWidgets/SpectrumViewer/ColorMaps.h"
#include "MantidQtWidgets/SpectrumViewer/GraphDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/TrackingPicker.h"
#include "ui_RefImageView.h"

/**
    @class RefIVConnections

    This class provides the connections between the SpectrumView GUI components
    made using QtDesigner and the classes that do the actual work for the
    SpectrumView.  It basically provides SLOTS that are called by the GUI
    components' SIGNALS and in turn call methods on the SpectrumView
    implementation objects.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace MantidQt {
namespace RefDetectorViewer {

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefIVConnections : public QWidget {
  Q_OBJECT

public:
  /// Construct the object that links the GUI components to the other specifed
  /// higher level objects.
  RefIVConnections(Ui_RefImageViewer *ui, RefImageView *imageView,
                   RefImageDisplay *imageDisplay,
                   SpectrumView::GraphDisplay *hGraphDisplay,
                   SpectrumView::GraphDisplay *vGraphDisplay);

  ~RefIVConnections() override;

  /// Set the pix map that shows the color scale from the specified color maps
  void showColorScale(std::vector<QRgb> &positiveColorTable,
                      std::vector<QRgb> &negativeColorTable);

public slots:
  void closeViewer();
  void toggleHScroll();
  void toggleVScroll();
  void imageHorizontalRangeChanged();
  void graphRangeChanged();
  void vScrollBarMoved();
  void hScrollBarMoved();
  void imageSplitterMoved();
  void imagePickerMoved();
  void imagePicker2Moved();
  void hGraphPickerMoved();
  void vGraphPickerMoved();
  void intensitySliderMoved();
  void editManualInput();
  void peakBackTofRangeUpdate();

  void heatColorScale();
  void grayColorScale();
  void negativeGrayColorScale();
  void greenYellowColorScale();
  void rainbowColorScale();
  void optimalColorScale();
  void multiColorScale();
  void spectrumColorScale();

signals:
  void peakBackTofRangeUpdate(double /*_t1*/, double /*_t2*/, double /*_t3*/, double /*_t4*/, double /*_t5*/, double /*_t6*/);

private:
  RefIVConnections() {}
  void setColorScale(MantidQt::SpectrumView::ColorMaps::ColorScale positive,
                     MantidQt::SpectrumView::ColorMaps::ColorScale negative);

private:
  Ui_RefImageViewer *m_ivUI;
  RefImageView *m_ivMainWindow;
  RefImageDisplay *m_imageDisplay;
  SpectrumView::GraphDisplay *m_hGraphDisplay;
  SpectrumView::GraphDisplay *m_vGraphDisplay;
  SpectrumView::TrackingPicker *m_imagePicker;
  SpectrumView::TrackingPicker *m_imagePicker2;
  SpectrumView::TrackingPicker *m_hGraphPicker;
  SpectrumView::TrackingPicker *m_vGraphPicker;
  QActionGroup *m_colorGroup;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_IV_CONNECTIONS_H
