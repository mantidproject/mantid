
#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_H_

#include "DllOption.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <QtGui/qwidget.h>
#include <qwt_plot.h>

namespace MantidQt
{
  namespace SliceViewer
  {
    /** Concrete view factory. For creating instances of PeakOverlay widget.

    @date 2012-08-24

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport PeakOverlayFactory : public PeakOverlayViewFactory
    {
    private:
      QwtPlot * m_plot;
      QWidget * m_parent;
      PeakDimensions m_peakDims;
    public:
      PeakOverlayFactory(QwtPlot * plot, QWidget * parent, const PeakDimensions peakDims);
      virtual ~PeakOverlayFactory();
      virtual boost::shared_ptr<PeakOverlayView> createView(const Mantid::API::IPeak&) const;
    };
  }
}

#endif /*MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_H_*/