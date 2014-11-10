#ifndef REF_RANGE_HANDLER_H
#define REF_RANGE_HANDLER_H

#include "MantidQtSpectrumViewer/IRangeHandler.h"
#include "ui_RefImageView.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "DllOption.h"

/**
    @class RangeHandler

    This manages the min, max and step range controls for the SpectrumView
    data viewer.

    @author Dennis Mikkelson
    @date   2012-04-25

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

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefRangeHandler : public SpectrumView::IRangeHandler
{
  public:

    /// Construct object to manage min, max and step controls in the UI
    RefRangeHandler( Ui_RefImageViewer* ivUI );

    /// Configure min, max and step controls for the specified data source
    void configureRangeControls( SpectrumView::SpectrumDataSource_sptr dataSource );

    /// Get the range of data to display in the image, from GUI controls
    void getRange( double &min, double &max, double &step );

    /// Set the values displayed in the GUI controls
    void setRange( double min, double max, double step, char type );

  private:
    Ui_RefImageViewer* m_ivUI;

    double m_totalMinX;
    double m_totalMaxX;
    double m_totalMinY;
    double m_totalMaxY;
    size_t m_totalNSteps;

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_RANGE_HANDLER_H
