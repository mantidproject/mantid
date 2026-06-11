// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class IBatchPresenter;
class RunsTable;

/// Presenter interface for synchronising the plotting tab with the batch GUI.
class IPlottingPresenter {
public:
  virtual ~IPlottingPresenter() = default;

  /// Attach the batch presenter so reduction state can control plotting controls.
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  /// Refresh plot output types available for the selected instrument.
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  /// Rebuild the plotting workspace tree after run-table or workspace changes.
  virtual void notifyRunsTableChanged(RunsTable const &runsTable) = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
