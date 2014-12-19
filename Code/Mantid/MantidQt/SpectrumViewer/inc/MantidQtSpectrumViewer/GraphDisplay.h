#ifndef  GRAPH_DISPLAY_H
#define  GRAPH_DISPLAY_H

#include <QTableWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class GraphDisplay

    This class handles the display of vertical and horizontal cuts
    through the data in an SpectrumView display.

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

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER GraphDisplay
{
  public:

   /// Construct a GraphDisplay to display in the specifed plot and table
   GraphDisplay( QwtPlot*      graphPlot,
                 QTableWidget* graphTable,
                 bool          isVertical );

   ~GraphDisplay();

   /// Set the source of information for the table of position information
   void setDataSource( SpectrumDataSource_sptr dataSource );

   /// Set the actual data that will be displayed on the graph
   void setData( const QVector<double> & xData,
                 const QVector<double> & yData,
                       double            cutValue );

   /// Clear the graph(s) off the display
   void clear();

   /// Set up axes using the specified scale factor and replot the graph
   void setRangeScale( double rangeScale );

   /// Set flag indicating whether or not to use a log scale on the x-axis
   void setLogX( bool isLogX );

   /// Record the point that the user is currently pointing at with the mouse
   void setPointedAtPoint( QPoint point );

  private:
   /// Show information about the point (x, y) on the graph, in the info table
   void showInfoList( double x, double y );

   QwtPlot            * m_graphPlot;
   QwtPlotCurve       * m_curve;
   QTableWidget       * m_graphTable;
   SpectrumDataSource_sptr m_dataSource;

   bool   m_isVertical;
   bool   m_isLogX;
   double m_imageX;
   double m_imageY;
   double m_rangeScale;  // Fraction of data range to be graphed
   double m_minX;
   double m_maxX;
   double m_minY;
   double m_maxY;

};

} // namespace SpectrumView
} // namespace MantidQt

#endif   // GRAPH_DISPLAY_H
