#ifndef  REF_IMAGE_VIEW_H
#define  REF_IMAGE_VIEW_H

#include <QMainWindow>
#include <QtGui>

#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "DllOption.h"

/**
    @class RefImageView

    This is the QMainWindow for the SpectrumView data viewer.  Data is
    displayed in an SpectrumView, by constructing the SpectrumView object and
    specifying a particular data source.

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

namespace Ui
{
class RefImageViewer;
}

namespace MantidQt
{
namespace RefDetectorViewer
{
class RefSliderHandler;
class RefRangeHandler;
class RefImageDisplay;
class RefIVConnections;

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageView : public QMainWindow
{
  public:

     /// Construct an RefImageView to display data from the specified data source
     RefImageView( SpectrumView::SpectrumDataSource_sptr dataSource,
                   int peakMin, int peakMax,
                   int backMin, int backMax,
                   int tofMin,  int tofMax);

    ~RefImageView();

    RefIVConnections* getIVConnections();

  private:
    SpectrumView::GraphDisplay* m_hGraph;
    SpectrumView::GraphDisplay* m_vGraph;

    Ui::RefImageViewer* m_ui;
    RefSliderHandler*   m_sliderHandler;
    RefRangeHandler*    m_rangeHandler;
    RefImageDisplay*    m_imageDisplay;
    RefIVConnections*   m_ivConnections;

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif   // REF_IMAGE_VIEW_H
