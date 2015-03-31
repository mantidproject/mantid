#ifndef MANTID_CUSTOMINTERFACES_IREFLPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLPRESENTER_H

#include <map>
#include <string>

#include "MantidKernel/System.h"

#include <QVariant>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class IReflPresenter

    IReflPresenter is an interface which defines the functions any reflectometry interface presenter needs to support.

    Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class IReflPresenter
    {
    public:
      virtual ~IReflPresenter() {};

      enum Flag
      {
        SaveFlag,
        SaveAsFlag,
        AppendRowFlag,
        PrependRowFlag,
        DeleteRowFlag,
        ProcessFlag,
        GroupRowsFlag,
        OpenTableFlag,
        NewTableFlag,
        TableUpdatedFlag,
        ExpandSelectionFlag,
        OptionsDialogFlag,
        ClearSelectedFlag,
        CopySelectedFlag,
        CutSelectedFlag,
        PasteSelectedFlag,
        SearchFlag,
        TransferFlag,
        ImportTableFlag,
        ExportTableFlag,
        PlotRowFlag,
        PlotGroupFlag
      };

      //Tell the presenter something happened
      virtual void notify(IReflPresenter::Flag flag) = 0;
      virtual const std::map<std::string,QVariant>& options() const = 0;
      virtual void setOptions(const std::map<std::string,QVariant>& options) = 0;
    };
  }
}
#endif
