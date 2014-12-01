#ifndef REF_IV_CONNECTIONS_H
#define REF_IV_CONNECTIONS_H

#include <QtCore/QtCore>
#include <QtGui/QWidget>
#include <QActionGroup>

#include "ui_RefImageView.h"
#include "MantidQtRefDetectorViewer/RefImageView.h"
#include "MantidQtSpectrumViewer/TrackingPicker.h"
#include "MantidQtRefDetectorViewer/RefImageDisplay.h"
#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "DllOption.h"

/**
    @class RefIVConnections

    This class provides the connections between the SpectrumView GUI components
    made using QtDesigner and the classes that do the actual work for the
    SpectrumView.  It basically provides SLOTS that are called by the GUI
    components' SIGNALS and in turn call methods on the SpectrumView
    implementation objects.

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace RefDetectorViewer
{

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefIVConnections: public QWidget
{
  Q_OBJECT

public:
  /// Construct the object that links the GUI components to the other specifed
  /// higher level objects.
  RefIVConnections( Ui_RefImageViewer*          ui,
                    RefImageView*               imageView,
                    RefImageDisplay*            imageDisplay,
                    SpectrumView::GraphDisplay* hGraphDisplay,
                    SpectrumView::GraphDisplay* vGraphDisplay );

  ~RefIVConnections();

  /// Set the pix map that shows the color scale from the specified color maps
  void showColorScale( std::vector<QRgb> & positiveColorTable,
                       std::vector<QRgb> & negativeColorTable );

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
  void peakBackTofRangeUpdate(double, double, double, double, double, double);

private:
  RefIVConnections() {}

private:

  Ui_RefImageViewer*            m_ivUI;
  RefImageView*                 m_ivMainWindow;
  RefImageDisplay*              m_imageDisplay;
  SpectrumView::GraphDisplay*   m_hGraphDisplay;
  SpectrumView::GraphDisplay*   m_vGraphDisplay;
  SpectrumView::TrackingPicker* m_imagePicker;
  SpectrumView::TrackingPicker* m_imagePicker2;
  SpectrumView::TrackingPicker* m_hGraphPicker;
  SpectrumView::TrackingPicker* m_vGraphPicker;
  QActionGroup*                 m_colorGroup;

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif  // REF_IV_CONNECTIONS_H
