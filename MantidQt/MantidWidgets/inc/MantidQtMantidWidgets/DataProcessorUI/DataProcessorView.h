#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H

#include "MantidKernel/System.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTreeModel.h"

#include <map>
#include <set>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
// Forward dec
class HintStrategy;
class DataProcessorPresenter;

/** @class DataProcessorView

DataProcessorView is the base view class for the Data Processor User
Interface. It contains no QT specific functionality as that should be handled by
a subclass.

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

class DLLExport DataProcessorView {
public:
  DataProcessorView(){};
  virtual ~DataProcessorView(){};

  // Connect the model
  virtual void showTable(QDataProcessorTreeModel_sptr model) = 0;

  // Dialog/Prompt methods
  virtual std::string requestNotebookPath() = 0;

  // Settings
  virtual void saveSettings(const std::map<std::string, QVariant> &options) = 0;
  virtual void loadSettings(std::map<std::string, QVariant> &options) = 0;

  // Get status of the checkbox which dictates whether an ipython notebook is
  // produced
  virtual bool getEnableNotebook() = 0;

  // Setter methods
  virtual void setTableList(const std::set<std::string> &tables) = 0;
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &defaultInstrument) = 0;
  virtual void setSelection(const std::set<int> &groups) = 0;
  virtual void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) = 0;
  virtual void setClipboard(const std::string &text) = 0;
  virtual void setModel(const std::string &name) = 0;

  // Accessor methods
  virtual std::map<int, std::set<int>> getSelectedRows() const = 0;
  virtual std::set<int> getSelectedGroups() const = 0;
  virtual std::string getWorkspaceToOpen() const = 0;
  virtual std::string getClipboard() const = 0;
  virtual std::string getProcessInstrument() const = 0;
  virtual DataProcessorPresenter *getPresenter() const = 0;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H*/
