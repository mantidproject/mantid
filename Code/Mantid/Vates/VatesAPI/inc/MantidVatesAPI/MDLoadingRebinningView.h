#ifndef MANTID_VATES_MD_REBINNING_LOADING_VIEW
#define MANTID_VATES_MD_REBINNING_LOADING_VIEW

#include "MantidVatesAPI/MDRebinningView.h"

//Forward declaration.
class vtkImplicitFunction;

namespace Mantid
{
  namespace VATES
  {

     /** 
    @class MDLoadingRebinningView. Abstract type. Adds extra access methods onto contract defined in MDRebinningView to support file loading.
    @author Owen Arnold, Tessella plc
    @date 25/07/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport MDLoadingRebinningView : public MDRebinningView
    {
    public:
      virtual bool getLoadInMemory() const = 0;
      virtual ~MDLoadingRebinningView(){}
    };
  }
}

#endif