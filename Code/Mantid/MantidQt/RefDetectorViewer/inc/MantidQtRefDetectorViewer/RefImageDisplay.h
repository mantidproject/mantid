#ifndef  REF_IMAGE_DISPLAY_H
#define  REF_IMAGE_DISPLAY_H

#include "MantidQtSpectrumViewer/ImageDisplay.h"
#include "MantidQtRefDetectorViewer/RefSliderHandler.h"
#include "MantidQtRefDetectorViewer/RefRangeHandler.h"
#include "MantidQtRefDetectorViewer/RefLimitsHandler.h"
#include "DllOption.h"

namespace MantidQt
{
namespace RefDetectorViewer
{

/** This class extends the SpectrumViewer::ImageDisplay class to communicate left-clicks
    to the RefLimitsHandler (and thence to the peak/background/TOF line edits in the gui)

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    Code Documentation is available at <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageDisplay : public SpectrumView::ImageDisplay
{
  public:
     /// Make an ImageDisplay to display with the given widgets and controls 
     RefImageDisplay( QwtPlot*       image_plot, 
                      RefSliderHandler* slider_handler,
                      RefRangeHandler*  range_handler,
                      RefLimitsHandler* limits_handler,
                      SpectrumView::GraphDisplay*  h_graph,
                      SpectrumView::GraphDisplay*  v_graph,
                      QTableWidget*  table_widget);

     ~RefImageDisplay();

     /// Record the point that the user is currently pointing at with the mouse
     /// default right click (mouseClick = 2)
     QPair<double,double> SetPointedAtPoint( QPoint point, int mouseClick = 2 );

private:
     RefLimitsHandler*    m_limitsHandler; // Owned by RefImagePlotItem
};

} // namespace RefDetectorViewer
} // namespace MantidQt 

#endif   // REF_IMAGE_DISPLAY_H
