#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_

#include "DllOption.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qcolor.h>

namespace MantidQt
{
  namespace SliceViewer
  {

    /** Base class for Concrete view factories. Provides common functionality. This is abstract.

    @date 2012-08-24

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport PeakOverlayViewFactoryBase : public PeakOverlayViewFactory
    {
    protected:
      QwtPlot * m_plot;
      QWidget * m_parent;
      QColor m_peakColour;
      QColor m_backColour;
    public:
      PeakOverlayViewFactoryBase(QwtPlot * plot, QWidget * parent, const size_t colourNumber=0);
      virtual ~PeakOverlayViewFactoryBase();
      virtual std::string getPlotXLabel() const;
      virtual std::string getPlotYLabel() const;
    };
  }
}

#endif /*MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_*/