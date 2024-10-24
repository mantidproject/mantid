// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class RunsTableViewSubscriber : public MantidQt::MantidWidgets::Batch::JobTreeViewSubscriber {
public:
  virtual void notifyResumeReductionRequested() = 0;
  virtual void notifyPauseReductionRequested() = 0;
  virtual void notifyInsertRowRequested() = 0;
  virtual void notifyInsertGroupRequested() = 0;
  virtual void notifyDeleteRowRequested() = 0;
  virtual void notifyDeleteGroupRequested() = 0;
  virtual void notifyFilterChanged(std::string const &filterValue) = 0;
  virtual void notifyChangeInstrumentRequested() = 0;
  virtual void notifyExpandAllRequested() = 0;
  virtual void notifyCollapseAllRequested() = 0;
  virtual void notifyPlotSelectedPressed() = 0;
  virtual void notifyPlotSelectedStitchedOutputPressed() = 0;
  virtual void notifyFillDown() = 0;

  virtual ~RunsTableViewSubscriber() = default;
};

/** @class IRunsTableView

IRunsTableView is the base view class for the table component of the "Runs" tab
in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL IRunsTableView {
public:
  enum class Action {
    Process,
    Pause,
    InsertRow,
    InsertGroup,
    DeleteRow,
    DeleteGroup,
    Copy,
    Paste,
    Cut,
    Expand,
    Collapse,
    PlotSelected,
    PlotSelectedStitchedOutput,
    FillDown
  };

  virtual void subscribe(RunsTableViewSubscriber *notifyee) = 0;
  virtual void setProgress(int value) = 0;
  virtual void resetFilterBox() = 0;
  virtual MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() = 0;

  virtual void invalidSelectionForCopy() = 0;
  virtual void invalidSelectionForPaste() = 0;
  virtual void invalidSelectionForCut() = 0;

  virtual void mustSelectRow() = 0;
  virtual void mustSelectGroup() = 0;
  virtual void mustNotSelectGroup() = 0;
  virtual void mustSelectGroupOrRow() = 0;

  virtual std::string getInstrumentName() const = 0;
  virtual void setInstrumentName(std::string const &instrumentName) = 0;

  virtual void setJobsTableEnabled(bool enable) = 0;
  virtual void setInstrumentSelectorEnabled(bool enable) = 0;
  virtual void setProcessButtonEnabled(bool enable) = 0;
  virtual void setActionEnabled(Action action, bool enable) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
