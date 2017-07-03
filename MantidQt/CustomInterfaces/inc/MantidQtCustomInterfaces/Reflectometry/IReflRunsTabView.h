#ifndef MANTID_CUSTOMINTERFACES_IREFLRUNSTABVIEW_H
#define MANTID_CUSTOMINTERFACES_IREFLRUNSTABVIEW_H

#include <set>
#include <string>
#include <boost/shared_ptr.hpp>

namespace MantidQt {

namespace MantidWidgets {
class DataProcessorCommand;
}
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

using MantidWidgets::DataProcessorCommand;
using API::AlgorithmRunner;
class IReflRunsTabPresenter;
class ReflSearchModel;

/** @class IReflRunsTabView

IReflRunsTabView is the base view class for the Reflectometry Interface. It
contains no QT specific functionality as that should be handled by a subclass.

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

class DLLExport IReflRunsTabView {
public:
  IReflRunsTabView(){};
  virtual ~IReflRunsTabView(){};

  // Connect the model
  virtual void showSearch(boost::shared_ptr<ReflSearchModel> model) = 0;

  // Setter methods
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &defaultInstrument) = 0;
  virtual void setTransferMethods(const std::set<std::string> &methods) = 0;
  virtual void setTableCommands(
      std::vector<std::unique_ptr<DataProcessorCommand>> tableCommands) = 0;
  virtual void setRowCommands(
      std::vector<std::unique_ptr<DataProcessorCommand>> rowCommands) = 0;
  virtual void setAllSearchRowsSelected() = 0;
  virtual void clearCommands() = 0;
  virtual void setRowActionEnabled(int index, bool enabled) = 0;
  virtual void setAutoreduceButtonEnabled(bool enabled) = 0;

  // Accessor methods
  virtual std::set<int> getSelectedSearchRows() const = 0;
  virtual std::string getSearchInstrument() const = 0;
  virtual std::string getSearchString() const = 0;
  virtual std::string getTransferMethod() const = 0;
  virtual int getSelectedGroup() const = 0;

  virtual IReflRunsTabPresenter *getPresenter() const = 0;
  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const = 0;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLRUNSTABVIEW_H */
