#ifndef  REF_IMAGE_DISPLAY_H
#define  REF_IMAGE_DISPLAY_H

#include "MantidQtSpectrumViewer/SpectrumDisplay.h"
#include "MantidQtRefDetectorViewer/RefSliderHandler.h"
#include "MantidQtRefDetectorViewer/RefRangeHandler.h"
#include "MantidQtRefDetectorViewer/RefLimitsHandler.h"
#include "DllOption.h"

namespace MantidQt
{
namespace RefDetectorViewer
{

/** This class extends the SpectrumViewer::SpectrumDisplay class to communicate left-clicks
    to the RefLimitsHandler (and thence to the peak/background/TOF line edits in the gui)

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageDisplay : public SpectrumView::SpectrumDisplay
{
  public:
     /// Make a SpectrumDisplay to display with the given widgets and controls
     RefImageDisplay( QwtPlot*                    imagePlot,
                      RefSliderHandler*           sliderHandler,
                      RefRangeHandler*            rangeHandler,
                      RefLimitsHandler*           limitsHandler,
                      SpectrumView::GraphDisplay* hGraph,
                      SpectrumView::GraphDisplay* vGraph,
                      QTableWidget*               tableWidget);

     ~RefImageDisplay();

     /// Record the point that the user is currently pointing at with the mouse
     /// default right click (mouseClick = 2)
     QPair<double,double> setPointedAtPoint( QPoint point, int mouseClick = 2 );

private:
     RefLimitsHandler*    m_limitsHandler; // Owned by RefImagePlotItem

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif   // REF_IMAGE_DISPLAY_H
