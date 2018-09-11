#ifndef SPECTRUM_DISPLAY_H
#define SPECTRUM_DISPLAY_H

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QTableWidget>
#include <boost/weak_ptr.hpp>
#include <qwt_plot.h>

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/GraphDisplay.h"
#include "MantidQtWidgets/SpectrumViewer/IRangeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/ISliderHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumPlotItem.h"

/**
    @class SpectrumDisplay

    This class provides the image display and coordinates the image and
    graph displays for the SpectrumView data viewer.

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

namespace MantidQt {
namespace SpectrumView {

class TrackingPicker;

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumDisplay : public QObject {
  Q_OBJECT

public:
  /// Make an SpectrumDisplay to display with the given widgets and controls
  SpectrumDisplay(QwtPlot *spectrumPlot, ISliderHandler *sliderHandler,
                  IRangeHandler *rangeHandler, GraphDisplay *hGraph,
                  GraphDisplay *vGraph, QTableWidget *tableWidget,
                  bool isTrackingOn = true);

  ~SpectrumDisplay() override;

  virtual bool hasData(const std::string &wsName,
                       const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// Set some properties of the SpectrumPlotItem object
  void setupSpectrumPlotItem();

  /// Set the source of the image data and information for the table
  void setDataSource(SpectrumDataSource_sptr dataSource);

  /// Rebuild the scroll bars and image due to change of xmin, xmax, step
  void updateRange();

  /// Updates scroll bars when window is resized
  void handleResize();

  /// Rebuild image from data source, due to resize or scroll bar movement
  void updateImage();

  /// Change the color tables used to map intensity to color
  void setColorScales(std::vector<QRgb> &positiveColorTable,
                      std::vector<QRgb> &negativeColorTable);

  /// Change the control parameter (0...100) used to brighten the image
  void setIntensity(double controlParameter);

  /// Record the point that the user is currently pointing at with the mouse
  virtual QPair<double, double>
  setPointedAtPoint(QPoint point, int mouseClick = 2, bool isFront = true);

  /// Record the point that the user is currently pointing in the scales
  /// coordinates
  virtual void setPointedAtXY(double x, double y, bool isFront = true);

  /// Set horizontal graph wit data from the array at the specified y value
  void setHGraph(double y, bool isFront = true);

  /// Set vertical graph with data from the array at the specified x value
  void setVGraph(double x, bool isFront = true);

  /// Show information about the point (x, y) on the image in the table
  std::vector<std::string> showInfoList(double x, double y);

  /// Gets a point on the graph area for a set of axis values
  QPoint getPlotTransform(QPair<double, double> values);

  /// Gets a set of axis values for a point on the graph area
  QPair<double, double> getPlotInvTransform(QPoint point);

  // Gets the X value pointed at
  double getPointedAtX();

  // Gets the Y value pointed at
  double getPointedAtY();

  QwtPlot *spectrumPlot() const { return m_spectrumPlot; }

  void addOther(const boost::shared_ptr<SpectrumDisplay> &other);
  void addOthers(const QList<boost::shared_ptr<SpectrumDisplay>> &others);
  void removeOther(const boost::shared_ptr<SpectrumDisplay> &other);

  void setTrackingOn(bool on);

protected:
  SpectrumPlotItem *m_spectrumPlotItem;

private slots:

  void imagePickerMoved(const QPoint &point);

private:
  /// Check if the DataSource has been changed under us
  bool dataSourceRangeChanged();

  /// Get the rectangle currently covered by the image in pixel coordinates
  void getDisplayRectangle(QRect &rect);

  std::vector<QRgb> m_positiveColorTable;
  std::vector<QRgb> m_negativeColorTable;
  std::vector<double> m_intensityTable;

  SpectrumDataSource_sptr m_dataSource;
  DataArray_const_sptr m_dataArray;

  QwtPlot *m_spectrumPlot;

  ISliderHandler *m_sliderHandler;
  IRangeHandler *m_rangeHandler;

  GraphDisplay *m_hGraphDisplay;
  GraphDisplay *m_vGraphDisplay;

  double m_pointedAtX;
  double m_pointedAtY;

  /* Save current total data range */
  /* so we can reset the data source */
  /* if we detect a change of range */
  QTableWidget *m_imageTable;

  double m_totalXMin;
  double m_totalXMax;
  double m_totalYMin;
  double m_totalYMax;

  QList<boost::weak_ptr<SpectrumDisplay>> m_otherDisplays;
  TrackingPicker *m_imagePicker;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // SPECTRUM_DISPLAY_H
