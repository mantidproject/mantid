#ifndef MANTID_SLICEVIEWER_ZOOMABLE_PEAKS_VIEW_H_
#define MANTID_SLICEVIEWER_ZOOMABLE_PEAKS_VIEW_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /// Forward dec
    class PeakBoundingBox;

    /** Abstract view in Representing a view that can be zoomed in upon.
    
    @date 2013-01-08

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
    class DLLExport ZoomablePeaksView
    {
    public:
      /// Zoom to a peak position provided by a boundary rectangle in the windows coordinate system.
      virtual void zoomToRectangle(const PeakBoundingBox&) = 0;
      /// Zoom out
      virtual void resetView() = 0;
      /// Detach
      virtual void detach() = 0;
      /// Destructor
      virtual ~ZoomablePeaksView(){ }
    };
  }
}

#endif /* MANTID_SLICEVIEWER_ZOOMABLE_PEAKS_VIEW_H_ */
