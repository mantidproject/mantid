#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYMULTICROSS_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYMULTICROSS_FACTORY_H_

#include "MantidQtSliceViewer/PeakOverlayViewFactoryBase.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MDGeometry.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {

    /** Concrete view factory. For creating instances of PeakOverlayMultiCross widget.

    @date 2013-06-10

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
    class DLLExport PeakOverlayMultiCrossFactory : public PeakOverlayViewFactoryBase
    {
      public:
      PeakOverlayMultiCrossFactory(boost::shared_ptr<Mantid::API::MDGeometry> mdWS, Mantid::API::PeakTransform_const_sptr transform, Mantid::API::IPeaksWorkspace_sptr peaksWS, QwtPlot * plot, QWidget * parent, const size_t colourNumber=0);
      virtual ~PeakOverlayMultiCrossFactory();
      virtual boost::shared_ptr<PeakOverlayView> createView(Mantid::API::PeakTransform_const_sptr transform) const;
      virtual int FOM() const;
      virtual void swapPeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace>& peaksWS);
    private:
      /// Peaks workspace.
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> m_peaksWS;
      double m_zMax;
      double m_zMin;
    };
  }
}
#endif
