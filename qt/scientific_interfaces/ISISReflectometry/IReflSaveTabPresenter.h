#ifndef MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "IReflBatchPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflSaveTabPresenter

IReflSaveTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Save ASCII' tab presenter

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class IReflSaveTabPresenter {
public:
  virtual ~IReflSaveTabPresenter(){};
  /// Accept a main presenter
  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;

  enum Flag {
    populateWorkspaceListFlag,
    filterWorkspaceListFlag,
    workspaceParamsFlag,
    saveWorkspacesFlag,
    suggestSaveDirFlag,
    autosaveEnabled,
    autosaveDisabled,
    savePathChanged
  };

  virtual void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) = 0;

  virtual void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) = 0;

  /// Tell the presenter something happened
  virtual void notify(IReflSaveTabPresenter::Flag flag) = 0;
  virtual void onAnyReductionPaused() = 0;
  virtual void onAnyReductionResumed() = 0;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H */
