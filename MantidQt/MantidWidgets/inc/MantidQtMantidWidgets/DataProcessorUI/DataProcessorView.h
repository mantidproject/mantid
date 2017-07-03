#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H

#include "MantidKernel/System.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <boost/shared_ptr.hpp>

class QAbstractItemModel;

namespace MantidQt {
namespace MantidWidgets {
// Forward dec
class HintStrategy;
class DataProcessorCommand;
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

  // Add actions to the toolbar
  virtual void
  addActions(std::vector<std::unique_ptr<DataProcessorCommand>> commands) = 0;

  // Connect the model
  virtual void showTable(boost::shared_ptr<QAbstractItemModel> model) = 0;

  // Dialog/Prompt methods
  virtual std::string requestNotebookPath() = 0;
  /// Dialog/Prompt methods
  virtual std::string askUserString(const std::string &prompt,
                                    const std::string &title,
                                    const std::string &defaultValue) = 0;
  virtual bool askUserYesNo(std::string prompt, std::string title) = 0;
  virtual void giveUserWarning(std::string prompt, std::string title) = 0;
  virtual void giveUserCritical(std::string prompt, std::string title) = 0;
  virtual std::string runPythonAlgorithm(const std::string &algorithm) = 0;

  // Settings
  virtual void saveSettings(const std::map<std::string, QVariant> &options) = 0;
  virtual void loadSettings(std::map<std::string, QVariant> &options) = 0;

  // Get status of the checkbox which dictates whether an ipython notebook is
  // produced
  virtual bool getEnableNotebook() = 0;

  // Expand/Collapse all groups
  virtual void expandAll() = 0;
  virtual void collapseAll() = 0;

  // Handle pause/resume of data reduction
  virtual void pause() = 0;
  virtual void resume() = 0;

  // Setter methods
  virtual void setTableList(const QSet<QString> &tables) = 0;
  virtual void setInstrumentList(const QString &instruments,
                                 const QString &defaultInstrument) = 0;
  virtual void setSelection(const std::set<int> &groups) = 0;
  virtual void setSelectionModelConnections() = 0;
  virtual void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) = 0;
  virtual void setClipboard(const std::string &text) = 0;
  virtual void setModel(const std::string &name) = 0;

  // Accessor methods
  virtual std::map<int, std::set<int>> getSelectedChildren() const = 0;
  virtual std::set<int> getSelectedParents() const = 0;
  virtual std::string getWorkspaceToOpen() const = 0;
  virtual std::string getClipboard() const = 0;
  virtual std::string getProcessInstrument() const = 0;
  virtual DataProcessorPresenter *getPresenter() const = 0;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORVIEW_H*/
