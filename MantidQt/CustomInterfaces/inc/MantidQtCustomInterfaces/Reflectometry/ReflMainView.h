#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEW_H

#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"

#include <set>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
// Forward decs
class ReflCommand;
using ReflCommand_uptr = std::unique_ptr<ReflCommand>;

/** @class ReflMainView

ReflMainView is the base view class for the Reflectometry Interface. It contains
no QT specific functionality as that should be handled by a subclass.

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

class DLLExport ReflMainView {
public:
  ReflMainView(){};
  virtual ~ReflMainView(){};

  // Connect the model
  virtual void showSearch(ReflSearchModel_sptr model) = 0;

  // Dialog/Prompt methods
  virtual std::string askUserString(const std::string &prompt,
                                    const std::string &title,
                                    const std::string &defaultValue) = 0;
  virtual void giveUserInfo(std::string prompt, std::string title) = 0;
  virtual void giveUserCritical(std::string prompt, std::string title) = 0;
  virtual void showAlgorithmDialog(const std::string &algorithm) = 0;

  // Setter methods
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &defaultInstrument) = 0;
  virtual void setTransferMethods(const std::set<std::string> &methods) = 0;
  virtual void
  setTableCommands(std::vector<ReflCommand_uptr> tableCommands) = 0;
  virtual void setRowCommands(std::vector<ReflCommand_uptr> rowCommands) = 0;
  virtual void clearCommands() = 0;

  // Accessor methods
  virtual std::set<int> getSelectedSearchRows() const = 0;
  virtual std::string getSearchInstrument() const = 0;
  virtual std::string getSearchString() const = 0;
  virtual std::string getTransferMethod() const = 0;

  virtual boost::shared_ptr<IReflPresenter> getPresenter() const = 0;
  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const = 0;
};
}
}
#endif