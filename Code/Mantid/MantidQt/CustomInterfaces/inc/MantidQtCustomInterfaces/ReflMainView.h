#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class ReflMainView

    ReflMainView is the base view class for the Reflectometry Interface. It contains no QT specific functionality as that should be handled by a subclass.

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

    class DLLExport ReflMainView
    {
    public:
      ReflMainView();
      virtual ~ReflMainView() = 0;

      //Connect the model
      virtual void showTable(Mantid::API::ITableWorkspace_sptr model) = 0;

      //dialog box methods
      virtual std::string getUserString() const = 0;
      virtual bool askUserString() = 0;
      virtual void giveUserInfo(std::string prompt, std::string title) = 0;
      virtual void giveUserWarning(std::string prompt, std::string title) = 0;
      virtual void giveUserCritical(std::string prompt, std::string title) = 0;
      virtual bool askUserYesNo(std::string prompt, std::string title) = 0;

      enum Flag
      {
        NoFlags = 0,
        SaveFlag,
        SaveAsFlag,
        AddRowFlag,
        DeleteRowFlag,
        ProcessFlag
      };

      //flag methods
      virtual std::vector<size_t> getSelectedRowIndexes() const = 0;
      virtual Flag getFlag() = 0;
      virtual bool flagSet() const = 0;
    };
  }
}
#endif
