// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflSaveTabPresenter

IReflSaveTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Save ASCII' tab presenter
*/
class IReflSaveTabPresenter {
public:
  virtual ~IReflSaveTabPresenter(){};
  /// Accept a main presenter
  virtual void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) = 0;

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
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSAVETABPRESENTER_H */
