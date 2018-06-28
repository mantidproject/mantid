/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
}
}
#endif // MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
