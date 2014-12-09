#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"
#include "MantidQtCustomInterfaces/ReflSearchModel.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtMantidWidgets/HintStrategy.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class ReflMainView

    ReflMainView is the base view class for the Reflectometry Interface. It contains no QT specific functionality as that should be handled by a subclass.

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

    class DLLExport ReflMainView
    {
    public:
      ReflMainView() {};
      virtual ~ReflMainView() {};

      //Connect the model
      virtual void showTable(QReflTableModel_sptr model) = 0;
      virtual void showSearch(ReflSearchModel_sptr model) = 0;

      //Dialog/Prompt methods
      virtual std::string askUserString(const std::string& prompt, const std::string& title, const std::string& defaultValue) = 0;
      virtual bool askUserYesNo(std::string prompt, std::string title) = 0;
      virtual void giveUserInfo(std::string prompt, std::string title) = 0;
      virtual void giveUserWarning(std::string prompt, std::string title) = 0;
      virtual void giveUserCritical(std::string prompt, std::string title) = 0;
      virtual void showAlgorithmDialog(const std::string& algorithm) = 0;

      //Plotting
      virtual void plotWorkspaces(const std::set<std::string>& workspaces) = 0;

      //Set the status of the progress bar
      virtual void setProgressRange(int min, int max) = 0;
      virtual void setProgress(int progress) = 0;

      //Settor methods
      virtual void setSelection(const std::set<int>& rows) = 0;
      virtual void setTableList(const std::set<std::string>& tables) = 0;
      virtual void setInstrumentList(const std::vector<std::string>& instruments, const std::string& defaultInstrument) = 0;
      virtual void setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy* hintStrategy) = 0;
      virtual void setClipboard(const std::string& text) = 0;

      //Accessor methods
      virtual std::set<int> getSelectedRows() const = 0;
      virtual std::set<int> getSelectedSearchRows() const = 0;
      virtual std::string getSearchInstrument() const = 0;
      virtual std::string getProcessInstrument() const = 0;
      virtual std::string getWorkspaceToOpen() const = 0;
      virtual std::string getClipboard() const = 0;
      virtual std::string getSearchString() const = 0;

      virtual boost::shared_ptr<IReflPresenter> getPresenter() const = 0;
    };
  }
}
#endif
