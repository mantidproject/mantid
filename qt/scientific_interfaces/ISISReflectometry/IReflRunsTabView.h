#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H

#include "MantidQtWidgets/Common/DataProcessorUI/EditAction.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TableAction.h"
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {

namespace MantidWidgets {
class DataProcessorCommand;
}
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

using MantidWidgets::DataProcessorCommand;
using MantidWidgets::EditAction;
using MantidWidgets::TableAction;
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
  using CommandVector = std::vector<std::unique_ptr<DataProcessorCommand>>;
  virtual void setReflectometryMenuCommands(const CommandVector& commands) = 0;
  virtual void setEditMenuCommands(const CommandVector& rowCommands) = 0;

  virtual void setAllSearchRowsSelected() = 0;
  virtual void clearCommands() = 0;
  virtual void enableEditMenuAction(int action) = 0;
  virtual void disableEditMenuAction(int action) = 0;
  virtual void enableReflectometryMenuAction(int action) = 0;
  virtual void disableReflectometryMenuAction(int action) = 0;
  virtual void enableTransfer() = 0;
  virtual void disableTransfer() = 0;
  virtual void enableAutoreduce() = 0;
  virtual void disableAutoreduce() = 0;

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
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABVIEW_H */
