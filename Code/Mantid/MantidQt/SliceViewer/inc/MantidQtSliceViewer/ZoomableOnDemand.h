#ifndef ZOOMABLEONDEMAND_H_
#define ZOOMABLEONDEMAND_H_

#include "MantidKernel/System.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /// Forward dec
    class PeaksPresenter;

    /** Abstract behavioural type for zooming to a peak region on demand.

     @date 2013-07-11

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

    class DLLExport ZoomableOnDemand
    {
    public:
      /// Zoom to a peak
      virtual void zoomToPeak(PeaksPresenter* const presenter, const int peakIndex) = 0;
      /// Reset/forget zoom
      virtual void resetZoom() = 0;
      /// destructor
      virtual ~ZoomableOnDemand(){}
    };

  }
}

#endif /* ZOOMABLEONDEMAND_H_ */
