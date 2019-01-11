// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SV_CONNECTIONS_H
#define SV_CONNECTIONS_H

#include <QActionGroup>
#include <QWidget>

#include "MantidQtWidgets/SpectrumViewer/ColorMaps.h"
#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/GraphDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumView.h"
#include "MantidQtWidgets/SpectrumViewer/TrackingPicker.h"
#include "ui_SpectrumView.h"

/**
    @class SVConnections

    This class provides the connections between the SpectrumView GUI components
    made using QtDesigner and the classes that do the actual work for the
    SpectrumView.  It basically provides SLOTS that are called by the GUI
    components' SIGNALS and in turn call methods on the SpectrumView
    implementation objects.

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SVConnections : public QWidget {
  Q_OBJECT

public:
  /// Construct the object that links the GUI components to the other specifed
  /// higher level objects.
  SVConnections(Ui_SpectrumViewer *ui, SpectrumView *spectrumView,
                SpectrumDisplay *spectrumDisplay, GraphDisplay *hGraphDisplay,
                GraphDisplay *vGraphDisplay);

  ~SVConnections() override;

  /// Set the pix map that shows the color scale from the specified color maps
  void showColorScale(std::vector<QRgb> &positiveColorTable,
                      std::vector<QRgb> &negativeColorTable);

  /// Get the applied color scales
  std::pair<ColorMaps::ColorScale, ColorMaps::ColorScale>
  getColorScales() const {
    return m_colorScales;
  }
  /// Get the color map file name
  QString getColorMapFileName() const { return m_colorMapFileName; }
  void setColorScale(ColorMaps::ColorScale positive,
                     ColorMaps::ColorScale negative);

public slots:
  void closeViewer();
  void toggleHScroll();
  void toggleVScroll();
  void imageHorizontalRangeChanged();
  void graphRangeChanged();
  void scrollBarMoved();
  void imageSplitterMoved();
  void vgraphSplitterMoved();
  void hGraphPickerMoved(const QPoint &point);
  void vGraphPickerMoved(const QPoint &point);
  void intensitySliderMoved();
  void loadColorMap(const QString &filename = "");
  void openOnlineHelp();

  void heatColorScale();
  void grayColorScale();
  void negativeGrayColorScale();
  void greenYellowColorScale();
  void rainbowColorScale();
  void optimalColorScale();
  void multiColorScale();
  void spectrumColorScale();

  void setSpectrumDisplay(SpectrumDisplay *spectrumDisplay);
  SpectrumDisplay *getCurrentSpectrumDisplay() const;
  void removeSpectrumDisplay(SpectrumDisplay *spectrumDisplay);

private:
  /// Event filter for mouse wheel capture
  bool eventFilter(QObject *object, QEvent *event) override;

  Ui_SpectrumViewer *m_svUI;
  SpectrumView *m_svMainWindow;
  QList<SpectrumDisplay *> m_spectrumDisplays;
  SpectrumDisplay *m_currentSpectrumDisplay;
  GraphDisplay *m_hGraphDisplay;
  GraphDisplay *m_vGraphDisplay;
  TrackingPicker *m_hGraphPicker;
  TrackingPicker *m_vGraphPicker;
  QActionGroup *m_colorGroup;
  std::pair<ColorMaps::ColorScale, ColorMaps::ColorScale> m_colorScales;
  QString m_colorMapFileName;

  /// Last known cursor position in the data (x-direction).
  int m_pickerX;

  /// Last known cursor position in the data (y-direction).
  int m_pickerY;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // SV_CONNECTIONS_H
