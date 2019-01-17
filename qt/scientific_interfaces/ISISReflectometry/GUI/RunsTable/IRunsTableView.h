// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
#define MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
#include "../../DllConfig.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"

namespace MantidQt {
namespace CustomInterfaces {

class RunsTableViewSubscriber
    : public MantidQt::MantidWidgets::Batch::JobTreeViewSubscriber {
public:
  virtual void notifyProcessRequested() = 0;
  virtual void notifyPauseRequested() = 0;
  virtual void notifyInsertRowRequested() = 0;
  virtual void notifyInsertGroupRequested() = 0;
  virtual void notifyDeleteRowRequested() = 0;
  virtual void notifyDeleteGroupRequested() = 0;
  virtual void notifyFilterChanged(std::string const &filterValue) = 0;
  virtual void notifyExpandAllRequested() = 0;
  virtual void notifyCollapseAllRequested() = 0;

  virtual ~RunsTableViewSubscriber() = default;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IRunsTableView {
public:
  virtual void invalidSelectionForCopy() = 0;
  virtual void invalidSelectionForPaste() = 0;

  virtual void invalidSelectionForCut() = 0;
  virtual void mustSelectRow() = 0;
  virtual void mustSelectGroup() = 0;
  virtual void mustNotSelectGroup() = 0;
  virtual void mustSelectGroupOrRow() = 0;

  virtual void subscribe(RunsTableViewSubscriber *notifyee) = 0;
  virtual void setProgress(int value) = 0;
  virtual void resetFilterBox() = 0;
  virtual MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
