// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtRunsTableView.h"
#include "Common/IndexOf.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>
#include <QString>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

QtRunsTableView::QtRunsTableView(std::vector<std::string> instruments)
    : m_jobs(), m_instruments(std::move(instruments)) {
  m_ui.setupUi(this);
  m_ui.progressBar->setRange(0, 100);
  m_jobs = std::make_unique<MantidQt::MantidWidgets::Batch::JobTreeView>(
      QStringList({"Run(s)", "Angle", "1st Trans Run(s)", "2nd Trans Run(s)", "Q min", "Q max", "dQ/Q", "Scale",
                   "Options", "Lookup Index"}),
      MantidQt::MantidWidgets::Batch::Cell(""), this);
  constexpr double runColScaleFactor = 1.5;
  m_jobs->setColumnWidth(0, int(m_jobs->columnWidth(0) * runColScaleFactor));
  m_ui.mainLayout->insertWidget(2, m_jobs.get());
  showAlgorithmPropertyHintsInOptionsColumn();
  addToolbarActions();
  m_jobs->addActions(m_ui.toolBar->actions());

  for (auto &&instrument : m_instruments)
    m_ui.instrumentSelector->addItem(QString::fromStdString(instrument));

  connect(m_ui.filterBox, SIGNAL(textChanged(QString const &)), this, SLOT(onFilterChanged(QString const &)));
  connect(m_ui.instrumentSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onInstrumentChanged(int)));

  // Set up the icon
  m_ui.processButton->setIcon(MantidQt::Icons::getIcon("mdi.sigma", "black", 1.3));
}

void QtRunsTableView::invalidSelectionForCopy() {
  QMessageBox::critical(this, "Bad selection for copy", "All selected rows must share a common group.");
}

void QtRunsTableView::invalidSelectionForPaste() {
  QMessageBox::critical(this, "Bad selection for paste", "Please ensure destination is the same depth and size");
}

void QtRunsTableView::invalidSelectionForCut() {
  QMessageBox::critical(this, "Bad selection for cut", "All selected rows must share a common group.");
}

void QtRunsTableView::mustSelectRow() {
  QMessageBox::critical(this, "No Row Selected", "To delete a row you must select one or more rows.");
}

void QtRunsTableView::mustSelectGroup() {
  QMessageBox::critical(this, "No Group Selected", "To insert a row you must select a group to add it to.");
}

void QtRunsTableView::mustNotSelectGroup() {
  QMessageBox::critical(this, "Group Selected", "To delete rows you should not select any groups.");
}

void QtRunsTableView::mustSelectGroupOrRow() {
  QMessageBox::critical(this, "No Group Or Row Selected", "You must select a group or a row to perform this action.");
}

void QtRunsTableView::onFilterChanged(QString const &filter) { m_notifyee->notifyFilterChanged(filter.toStdString()); }

void QtRunsTableView::onInstrumentChanged(int index) {
  UNUSED_ARG(index);
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTable", "InstrumentChanged"}, false);
  m_notifyee->notifyChangeInstrumentRequested();
}

std::string QtRunsTableView::getInstrumentName() const { return m_ui.instrumentSelector->currentText().toStdString(); }

void QtRunsTableView::setInstrumentName(std::string const &instrumentName) {
  setSelected(*m_ui.instrumentSelector, instrumentName);
}

void QtRunsTableView::resetFilterBox() { m_ui.filterBox->clear(); }

void QtRunsTableView::showAlgorithmPropertyHintsInOptionsColumn() {
  auto constexpr optionsColumn = 8;
  m_jobs->setHintsForColumn(
      optionsColumn, std::make_unique<MantidQt::MantidWidgets::AlgorithmHintStrategy>(
                         "ReflectometryReductionOneAuto",
                         std::vector<std::string>{
                             "ThetaIn", "ThetaOut", "InputWorkspace", "OutputWorkspace", "OutputWorkspaceBinned",
                             "OutputWorkspaceWavelength", "FirstTransmissionRun", "SecondTransmissionRun",
                             "MomentumTransferMin", "MomentumTransferMax", "MomentumTransferStep", "ScaleFactor"}));
}

void QtRunsTableView::setJobsTableEnabled(bool enabled) {
  static const auto editTriggers = m_jobs->editTriggers();

  if (enabled)
    m_jobs->setEditTriggers(editTriggers);
  else
    m_jobs->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void QtRunsTableView::setActionEnabled(Action action, bool enable) { m_actions[action]->setEnabled(enable); }

void QtRunsTableView::setInstrumentSelectorEnabled(bool enable) { m_ui.instrumentSelector->setEnabled(enable); }

void QtRunsTableView::setProcessButtonEnabled(bool enable) { m_ui.processButton->setEnabled(enable); }

/**
 * Returns a toolbar item/action
 * @param action
 * @param icon For Qt4 The IconPath i.e. Mantidplot. In Qt5 uses it to get the
 * Icon from the mantidqt.icons library.
 * @param description
 * @return QAction*
 */
QAction *QtRunsTableView::addToolbarItem(Action action, std::string const &icon, std::string const &description) {
  QIcon qIcon;
  if (icon == "") {
    qIcon = QIcon(QString::fromStdString(""));
  } else {
    qIcon = MantidQt::Icons::getIcon(QString::fromStdString(icon));
  }

  m_actions[action] = m_ui.toolBar->addAction(qIcon, QString::fromStdString(description));
  return m_actions[action];
}

void QtRunsTableView::addToolbarActions() {
  connect(addToolbarItem(Action::Process, "mdi.sigma", "Process selected runs"), SIGNAL(triggered(bool)), this,
          SLOT(onProcessPressed(bool)));

  connect(addToolbarItem(Action::Pause, "mdi.pause", "Pause processing of runs"), SIGNAL(triggered(bool)), this,
          SLOT(onPausePressed(bool)));

  connect(addToolbarItem(Action::Expand, "mdi.expand-all", "Expand all groups"), SIGNAL(triggered(bool)), this,
          SLOT(onExpandAllGroupsPressed(bool)));

  connect(addToolbarItem(Action::Collapse, "mdi.collapse-all", "Collapse all groups"), SIGNAL(triggered(bool)), this,
          SLOT(onCollapseAllGroupsPressed(bool)));

  connect(addToolbarItem(Action::PlotSelected, "mdi.chart-line", "Plot selected rows as graphs"),
          SIGNAL(triggered(bool)), this, SLOT(onPlotSelectedPressed(bool)));

  connect(addToolbarItem(Action::PlotSelectedStitchedOutput, "mdi.chart-areaspline",
                         "Plot selected rows with stitched outputs as graphs"),
          SIGNAL(triggered(bool)), this, SLOT(onPlotSelectedStitchedOutputPressed(bool)));

  connect(addToolbarItem(Action::InsertRow, "mdi.table-row-plus-after", "Insert row into selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertRowPressed(bool)));

  connect(addToolbarItem(Action::DeleteRow, "mdi.table-row-remove", "Delete all selected rows"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteRowPressed(bool)));

  connect(addToolbarItem(Action::InsertGroup, "mdi.table-plus", "Insert group after first selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertGroupPressed(bool)));

  connect(addToolbarItem(Action::DeleteGroup, "mdi.table-remove", "Delete all selected groups"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteGroupPressed(bool)));

  connect(addToolbarItem(Action::Copy, "mdi.content-copy", "Copy the current selection"), SIGNAL(triggered(bool)), this,
          SLOT(onCopyPressed(bool)));

  connect(addToolbarItem(Action::Paste, "mdi.content-paste", "Paste over the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onPastePressed(bool)));

  connect(addToolbarItem(Action::Cut, "mdi.content-cut", "Cut the current selection"), SIGNAL(triggered(bool)), this,
          SLOT(onCutPressed(bool)));

  // In case of FillUp use "mdi.arrow-expand-up"
  connect(addToolbarItem(Action::FillDown, "mdi.arrow-expand-down", "Fill down selected rows for selected column"),
          SIGNAL(triggered(bool)), this, SLOT(onFillDownPressed(bool)));
}

MantidQt::MantidWidgets::Batch::IJobTreeView &QtRunsTableView::jobs() { return *m_jobs; }

void QtRunsTableView::subscribe(RunsTableViewSubscriber *notifyee) {
  m_notifyee = notifyee;
  m_jobs->subscribe(notifyee);
  connect(m_ui.processButton, SIGNAL(clicked(bool)), this, SLOT(onProcessPressed(bool)));
}

void QtRunsTableView::setProgress(int value) { m_ui.progressBar->setValue(value); }

void QtRunsTableView::onExpandAllGroupsPressed(bool) { m_notifyee->notifyExpandAllRequested(); }

void QtRunsTableView::onCollapseAllGroupsPressed(bool) { m_notifyee->notifyCollapseAllRequested(); }

void QtRunsTableView::onProcessPressed(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTable", "StartProcessing"}, false);
  m_notifyee->notifyResumeReductionRequested();
}

void QtRunsTableView::onPausePressed(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTable", "PauseProcessing"}, false);
  m_notifyee->notifyPauseReductionRequested();
}

void QtRunsTableView::onInsertRowPressed(bool) { m_notifyee->notifyInsertRowRequested(); }

void QtRunsTableView::onInsertGroupPressed(bool) { m_notifyee->notifyInsertGroupRequested(); }

void QtRunsTableView::onDeleteRowPressed(bool) { m_notifyee->notifyDeleteRowRequested(); }

void QtRunsTableView::onDeleteGroupPressed(bool) { m_notifyee->notifyDeleteGroupRequested(); }

void QtRunsTableView::onCopyPressed(bool) { m_notifyee->notifyCopyRowsRequested(); }

void QtRunsTableView::onCutPressed(bool) { m_notifyee->notifyCutRowsRequested(); }

void QtRunsTableView::onPastePressed(bool) { m_notifyee->notifyPasteRowsRequested(); }

void QtRunsTableView::onPlotSelectedPressed(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(Mantid::Kernel::FeatureType::Feature,
                                                                {"ISIS Reflectometry", "RunsTable", "PlotRows"}, false);
  m_notifyee->notifyPlotSelectedPressed();
}

void QtRunsTableView::onPlotSelectedStitchedOutputPressed(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTable", "PlotGroups"}, false);
  m_notifyee->notifyPlotSelectedStitchedOutputPressed();
}

void QtRunsTableView::onFillDownPressed(bool) { m_notifyee->notifyFillDown(); }

/** Set a combo box to the given value
 */
void QtRunsTableView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

RunsTableViewFactory::RunsTableViewFactory(std::vector<std::string> instruments)
    : m_instruments(std::move(instruments)) {}

QtRunsTableView *RunsTableViewFactory::operator()() const { return new QtRunsTableView(m_instruments); }

int RunsTableViewFactory::indexOfElseFirst(std::string const &instrument) const {
  return indexOf(m_instruments, [&instrument](std::string const &inst) { return instrument == inst; }).value_or(0);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
