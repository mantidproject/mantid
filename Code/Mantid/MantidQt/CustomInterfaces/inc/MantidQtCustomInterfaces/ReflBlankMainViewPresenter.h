#ifndef MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTER_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** ReflBlankMainViewPresenter : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport ReflBlankMainViewPresenter: public ReflMainViewPresenter
    {
    public:

      ReflBlankMainViewPresenter(ReflMainView* view);
      virtual ~ReflBlankMainViewPresenter();
    protected:
      //press changes to a previously saved-to item in the ADS, or ask for a name if never given one
      virtual void save();
      //press changes to a new item in the ADS
      virtual void saveAs();
    };


  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTER_H_ */