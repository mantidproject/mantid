#ifndef MANTID_SLICEVIEWER_ZOOMABLE_PEAKS_VIEW_H_
#define MANTID_SLICEVIEWER_ZOOMABLE_PEAKS_VIEW_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"

namespace MantidQt
{
  namespace SliceViewer
  {

    /** Abstract view in Representing a view that can be zoomed in upon.
    
    @date 2013-01-08

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport ZoomablePeaksView
    {
    public:
      /// Zoom to a peak position provided by a boundary rectangle in the windows coordinate system.
      virtual void zoomToRectangle(Mantid::Kernel::V2D& lowerLeft, Mantid::Kernel::V2D& upperRight) = 0;
      /// Destructor
      virtual ~ZoomablePeaksView(){ }
    };

    /**
    @class ZoomableAdapter
    Templated adapter to zoom to peak. Alows objects from outside this type hierachy to be made to work seamlessly with it.
    */
    template <class Adaptee>
    class DLLExport ZoomableAdapter : public ZoomablePeaksView
    {
    private:
      Adaptee * const _adaptee;
      ZoomableAdapter& operator=(const ZoomableAdapter& other);
      ZoomableAdapter(const ZoomableAdapter& other);
    public:
      ZoomableAdapter(Adaptee* const adaptee) : _adaptee(adaptee)
      {
      }

      void zoomToRectangle(Mantid::Kernel::V2D& lowerLeft, Mantid::Kernel::V2D& upperRight)
      {
        _adaptee.zoomToRectange(lowerLeft, upperRight);
      }
      virtual ~ZoomableAdapter(){ }
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_ */