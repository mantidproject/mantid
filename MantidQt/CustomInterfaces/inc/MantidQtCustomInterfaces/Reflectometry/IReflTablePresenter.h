#ifndef MANTID_CUSTOMINTERFACES_IREFLTABLEPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLTABLEPRESENTER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommand.h"

#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
// Forward decs
class WorkspaceReceiver;
class ReflCommand;
using ReflCommand_uptr = std::unique_ptr<ReflCommand>;

/** @class IReflTablePresenter

IReflTablePresenter is an interface which defines the functions any data
processor interface presenter needs to support.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class IReflTablePresenter {
public:
  virtual ~IReflTablePresenter(){};

  enum Flag {
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
    ImportTableFlag,
    ExportTableFlag,
    PlotRowFlag,
    PlotGroupFlag
  };

  // Tell the presenter something happened
  virtual void notify(IReflTablePresenter::Flag flag) = 0;
  virtual const std::map<std::string, QVariant> &options() const = 0;
  virtual void setOptions(const std::map<std::string, QVariant> &options) = 0;
  virtual void
  transfer(const std::vector<std::map<std::string, std::string>> &runs) = 0;
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &defaultInstrument) = 0;
  virtual std::vector<ReflCommand_uptr> publishCommands() = 0;
  virtual void accept(WorkspaceReceiver *workspaceReceiver) = 0;
  virtual void setModel(std::string name) = 0;
};
}
}
#endif