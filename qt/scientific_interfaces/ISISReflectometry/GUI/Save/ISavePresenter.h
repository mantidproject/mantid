// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IBatchPresenter;

/** @class ISavePresenter

ISavePresenter is an interface which defines the functions that need
to be implemented by a concrete 'Save ASCII' tab presenter
*/
class ISavePresenter {
public:
  virtual ~ISavePresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void saveWorkspaces(std::vector<std::string> const &workspaceNames, bool const isAutoSave) = 0;
  virtual bool shouldAutosave() const = 0;
  virtual bool shouldAutosaveGroupRows() const = 0;

  /// Tell the presenter something happened
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
