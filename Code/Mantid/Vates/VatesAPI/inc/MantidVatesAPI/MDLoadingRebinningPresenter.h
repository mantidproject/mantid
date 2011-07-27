#ifndef MANTID_VATES_MD_LOADING_REBINNING_PRESENTER
#define MANTID_VATES_MD_LOADING_REBINNING_PRESENTER

#include "MantidVatesAPI/MDRebinningPresenter.h"

class vtkUnstructuredGrid;
namespace Mantid
{
  namespace VATES
  {
     /** 
    @class MDRebinningPresenter, Abstract presenters for multi-dimensional rebinning of various types. Extends the responsibilities of the base type to handle file loading responsibilities.
    @author Owen Arnold, Tessella plc
    @date 15/07/2011

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
    class DLLExport MDLoadingRebinningPresenter : public MDRebinningPresenter
    {
    public:
      virtual bool canLoadFile() const = 0;
	    virtual void executeLoad(ProgressAction& eventHandler) = 0;
      virtual ~MDLoadingRebinningPresenter(){}
    };

  }
}

#endif