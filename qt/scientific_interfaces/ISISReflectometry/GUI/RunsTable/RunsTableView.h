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
#ifndef MANTID_CUSTOMINTERFACES_RUNSTABLEVIEW_H_
#define MANTID_CUSTOMINTERFACES_RUNSTABLEVIEW_H_
#include "DllConfig.h"
#include "IBatchView.h"
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "ui_BatchView.h"
#include <memory>
#include <vector>
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "IRunsTableView.h"
#include "ui_RunsTableView.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTableView : public QWidget,
                                                     public IRunsTableView {
  Q_OBJECT
public:
  explicit RunsTableView(std::vector<std::string> const &instruments,
                         int defaultInstrumentIndex);
  void subscribe(RunsTableViewSubscriber *notifyee) override;
  void setProgress(int value) override;
  void resetFilterBox() override;
  MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() override;

  void invalidSelectionForCopy() override;
  void invalidSelectionForPaste() override;
  void invalidSelectionForCut() override;
  void mustSelectRow() override;
  void mustSelectGroup() override;
  void mustNotSelectGroup() override;
  void mustSelectGroupOrRow() override;

private slots:
  void onProcessPressed(bool);
  void onPausePressed(bool);
  void onExpandAllGroupsPressed(bool);
  void onCollapseAllGroupsPressed(bool);
  void onInsertRowPressed(bool);
  void onInsertGroupPressed(bool);
  void onDeleteRowPressed(bool);
  void onDeleteGroupPressed(bool);
  void onCopyPressed(bool);
  void onCutPressed(bool);
  void onPastePressed(bool);
  void onFilterChanged(QString const &);

private:
  void addToolbarActions();
  QAction *addToolbarItem(std::string const &iconPath,
                          std::string const &description);
  void showAlgorithmPropertyHintsInOptionsColumn();
  Ui::RunsTableView m_ui;
  std::unique_ptr<MantidQt::MantidWidgets::Batch::JobTreeView> m_jobs;
  std::vector<std::string> m_instruments;
  RunsTableViewSubscriber *m_notifyee;
};

class RunsTableViewFactory {
public:
  explicit RunsTableViewFactory(std::vector<std::string> const &instruments);
  RunsTableView *operator()(int defaultInstrumentIndex) const;
  RunsTableView *operator()() const;
  int defaultInstrumentFromConfig() const;
  int indexOfElseFirst(std::string const &instrument) const;

private:
  std::vector<std::string> m_instruments;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_RUNSTABLEVIEW_H_
