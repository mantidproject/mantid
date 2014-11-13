#ifndef SV_CONNECTIONS_H
#define SV_CONNECTIONS_H

#include <QtCore/QtCore>
#include <QtGui/QWidget>
#include <QActionGroup>

#include "ui_SpectrumView.h"
#include "MantidQtSpectrumViewer/SpectrumView.h"
#include "MantidQtSpectrumViewer/TrackingPicker.h"
#include "MantidQtSpectrumViewer/SpectrumDisplay.h"
#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"


/**
    @class SVConnections

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
namespace SpectrumView
{

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SVConnections: public QWidget
{
  Q_OBJECT

public:

  /// Construct the object that links the GUI components to the other specifed
  /// higher level objects.
  SVConnections( Ui_SpectrumViewer* ui,
                 SpectrumView*      spectrumView,
                 SpectrumDisplay*   spectrumDisplay,
                 GraphDisplay*      hGraphDisplay,
                 GraphDisplay*      vGraphDisplay );

  ~SVConnections();

  /// Set the pix map that shows the color scale from the specified color maps
  void showColorScale( std::vector<QRgb> & positiveColorTable,
                       std::vector<QRgb> & negativeColorTable );

public slots:
  void closeViewer();
  void toggleHScroll();
  void toggleVScroll();
  void imageHorizontalRangeChanged();
  void graphRangeChanged();
  void scrollBarMoved();
  void imageSplitterMoved();
  void vgraphSplitterMoved();
  void imagePickerMoved(const QPoint &point);
  void hGraphPickerMoved(const QPoint &point);
  void vGraphPickerMoved(const QPoint &point);
  void intensitySliderMoved();
  void loadColorMap();
  void openOnlineHelp();

  void heatColorScale();
  void grayColorScale();
  void negativeGrayColorScale();
  void greenYellowColorScale();
  void rainbowColorScale();
  void optimalColorScale();
  void multiColorScale();
  void spectrumColorScale();

private:
  /// Event filter for mouse wheel capture
  bool eventFilter(QObject *object, QEvent *event);

  Ui_SpectrumViewer*  m_svUI;
  SpectrumView*       m_svMainWindow;
  SpectrumDisplay*    m_spectrumDisplay;
  GraphDisplay*       m_hGraphDisplay;
  GraphDisplay*       m_vGraphDisplay;
  TrackingPicker*     m_imagePicker;
  TrackingPicker*     m_hGraphPicker;
  TrackingPicker*     m_vGraphPicker;
  QActionGroup*       m_colorGroup;

  /// Last known cursor position in the data (x-direction).
  int m_pickerX;

  /// Last known cursor position in the data (y-direction).
  int m_pickerY;

};

} // namespace SpectrumView
} // namespace MantidQt

#endif  // SV_CONNECTIONS_H
