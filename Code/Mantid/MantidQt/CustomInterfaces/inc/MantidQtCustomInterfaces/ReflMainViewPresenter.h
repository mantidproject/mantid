#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"
namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class ReflMainViewPresenter

    ReflMainViewPresenter is a presenter class for teh Reflectometry Interface. It handles any interface functionality and model manipulation.

    Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class DLLExport ReflMainViewPresenter: public IReflPresenter
    {
    public:
      ReflMainViewPresenter(Mantid::API::ITableWorkspace_sptr model, ReflMainView* view);
      ReflMainViewPresenter(std::string model, ReflMainView* view);
      virtual ~ReflMainViewPresenter();
      virtual void load();
    private:

    };
  }
}
#endif