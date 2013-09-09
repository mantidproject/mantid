#ifndef  GRAPH_DISPLAY_H
#define  GRAPH_DISPLAY_H

#include <QTableWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtImageViewer/DllOptionIV.h"

/**
    @class GraphDisplay 
  
       This class handles the display of vertical and horizontal cuts
    through the data in an ImageView display.
 
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
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER GraphDisplay 
{
  public:

   /// Construct a GraphDisplay to display in the specifed plot and table    
   GraphDisplay( QwtPlot*      graph_plot, 
                 QTableWidget* graph_table,
                 bool          is_vertical );

  ~GraphDisplay();

   /// Set the source of information for the table of position information 
   void SetDataSource( ImageDataSource* data_source );

   /// Set the actual data that will be displayed on the graph
   void SetData( const QVector<double> & xData,
                 const QVector<double> & yData,
                       double            cut_value );

   /// Clear the graph(s) off the display
   void Clear();

   /// Set up axes using the specified scale factor and replot the graph 
   void SetRangeScale( double range_scale );

   /// Set flag indicating whether or not to use a log scale on the x-axis
   void SetLogX( bool is_log_x );

   /// Record the point that the user is currently pointing at with the mouse
   void SetPointedAtPoint( QPoint point );

  private:
   /// Show information about the point (x, y) on the graph, in the info table
   void ShowInfoList( double x, double y );

   QwtPlot*          graph_plot;
   QwtPlotCurve*     curve;
   QTableWidget*     graph_table;
   ImageDataSource*  data_source;

   bool    is_vertical;
   bool    is_log_x;
   double  image_x;
   double  image_y;
   double  range_scale;         // fraction of data range to be graphed  
   double  min_x,
           max_x;
   double  min_y,
           max_y;
};

} // namespace MantidQt 
} // namespace ImageView 


#endif   // GRAPH_DISPLAY_H
