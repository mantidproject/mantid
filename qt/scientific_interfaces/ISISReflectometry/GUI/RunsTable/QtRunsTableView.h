// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "IRunsTableView.h"
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "ui_RunsTableWidget.h"
#include <memory>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL QtRunsTableView : public QWidget, public IRunsTableView {
  Q_OBJECT
public:
  explicit QtRunsTableView(std::vector<std::string> instruments);
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

  std::string getInstrumentName() const override;
  void setInstrumentName(std::string const &instrumentName) override;

  void setJobsTableEnabled(bool enable) override;
  void setInstrumentSelectorEnabled(bool enable) override;
  void setProcessButtonEnabled(bool enable) override;
  void setActionEnabled(Action action, bool enable) override;

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
  void onInstrumentChanged(int index);
  void onPlotSelectedPressed(bool);
  void onPlotSelectedStitchedOutputPressed(bool);
  void onFillDownPressed(bool);

private:
  void addToolbarActions();
  QAction *addToolbarItem(Action action, std::string const &iconPath, std::string const &description);
  void showAlgorithmPropertyHintsInOptionsColumn();
  void setSelected(QComboBox &box, std::string const &str);
  void setEnabledStateForAllWidgets(bool enabled);

  Ui::RunsTableWidget m_ui;
  std::unique_ptr<MantidQt::MantidWidgets::Batch::JobTreeView> m_jobs;
  std::vector<std::string> m_instruments;
  RunsTableViewSubscriber *m_notifyee;
  std::map<Action, QAction *> m_actions;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};

class RunsTableViewFactory {
public:
  explicit RunsTableViewFactory(std::vector<std::string> instruments);
  QtRunsTableView *operator()() const;
  int defaultInstrumentFromConfig() const;
  int indexOfElseFirst(std::string const &instrument) const;

private:
  std::vector<std::string> m_instruments;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
