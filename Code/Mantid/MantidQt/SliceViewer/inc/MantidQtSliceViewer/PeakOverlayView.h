#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_

#include "MantidKernel/System.h"
#include <QPointF>

namespace MantidQt
{
  namespace SliceViewer
  {
    /// Enum describing types of peak dimensions.
    enum PeakDimensions{LabView, SampleView, HKLView};

    /** Abstract view in MVP model representing a PeakOverlay.
    
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
    class DLLExport PeakOverlayView
    {
    public:
      /// Set the position of the slice point.
      virtual void setSlicePoint(const double&) = 0; 
      /// Update the view.
      virtual void updateView() = 0;
      /// Destructor
      virtual ~PeakOverlayView()
      {
      }
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_H_ */