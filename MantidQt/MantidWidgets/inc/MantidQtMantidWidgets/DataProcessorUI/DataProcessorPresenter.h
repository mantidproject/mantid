#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPRESENTER_H

#include <QVariant>
#include <map>
#include <set>
#include <string>
#include <vector>

using ParentItems = std::set<int>;
using ChildItems = std::map<int, std::set<int>>;

namespace MantidQt {
namespace MantidWidgets {
// Forward decs
class DataProcessorCommand;
class DataProcessorMainPresenter;
class DataProcessorView;
class ProgressableView;

/** @class DataProcessorPresenter

DataProcessorPresenter is an interface which defines the functions any data
processor interface presenter needs to support.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DataProcessorPresenter {
public:
  virtual ~DataProcessorPresenter(){};

  enum Flag {
    SaveFlag,
    SaveAsFlag,
    AppendRowFlag,
    AppendGroupFlag,
    DeleteRowFlag,
    DeleteGroupFlag,
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
    PlotGroupFlag,
    ExpandAllGroupsFlag,
    CollapseAllGroupsFlag
  };

  // Tell the presenter something happened
  virtual void notify(DataProcessorPresenter::Flag flag) = 0;
  virtual const std::map<std::string, QVariant> &options() const = 0;
  virtual void setOptions(const std::map<std::string, QVariant> &options) = 0;
  virtual void
  transfer(const std::vector<std::map<std::string, std::string>> &runs) = 0;
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &defaultInstrument) = 0;
  virtual std::vector<std::unique_ptr<DataProcessorCommand>>
  publishCommands() = 0;
  virtual void accept(DataProcessorMainPresenter *mainPresenter) = 0;
  virtual void acceptViews(DataProcessorView *tableView,
                           ProgressableView *progressView) = 0;
  virtual void setModel(std::string name) = 0;
  virtual ParentItems selectedParents() const = 0;
  virtual ChildItems selectedChildren() const = 0;
  virtual bool askUserYesNo(const std::string &prompt,
                            const std::string &title) const = 0;
  virtual void giveUserWarning(const std::string &prompt,
                               const std::string &title) const = 0;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPRESENTER_H*/
