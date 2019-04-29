// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTableView.h"
#include "../Runs/RunsView.h"
#include "Common/IndexOf.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

RunsTableView::RunsTableView(std::vector<std::string> const &instruments,
                             int defaultInstrumentIndex)
    : m_jobs(), m_instruments(instruments) {
  m_ui.setupUi(this);
  m_jobs =
      Mantid::Kernel::make_unique<MantidQt::MantidWidgets::Batch::JobTreeView>(
          QStringList({"Run(s)", "Angle", "First Transmission Run",
                       "Second Transmission Run", "Q min", "Q max", "dQ/Q",
                       "Scale", "Options"}),
          MantidQt::MantidWidgets::Batch::Cell(""), this);
  m_ui.mainLayout->insertWidget(2, m_jobs.get());
  showAlgorithmPropertyHintsInOptionsColumn();
  addToolbarActions();
  m_jobs->addActions(m_ui.toolBar->actions());

  for (auto &&instrument : m_instruments)
    m_ui.instrumentSelector->addItem(QString::fromStdString(instrument));
  m_ui.instrumentSelector->setCurrentIndex(defaultInstrumentIndex);

  connect(m_ui.filterBox, SIGNAL(textEdited(QString const &)), this,
          SLOT(onFilterChanged(QString const &)));
  connect(m_ui.instrumentSelector, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onInstrumentChanged(int)));

  // Set up the icon
  m_ui.processButton->setIcon(
      MantidQt::Icons::getIcon("mdi.sigma", "black", 1.3));
}

void RunsTableView::invalidSelectionForCopy() {
  QMessageBox::critical(this, "Bad selection for copy",
                        "All selected rows must share a common group.");
}

void RunsTableView::invalidSelectionForPaste() {
  QMessageBox::critical(this, "Bad selection for paste",
                        "All selected rows must share a common group.");
}

void RunsTableView::invalidSelectionForCut() {
  QMessageBox::critical(this, "Bad selection for cut",
                        "All selected rows must share a common group.");
}

void RunsTableView::mustSelectRow() {
  QMessageBox::critical(this, "No Row Selected",
                        "To delete a row you must select one or more rows.");
}

void RunsTableView::mustSelectGroup() {
  QMessageBox::critical(
      this, "No Group Selected",
      "To insert a row you must select a group to add it to.");
}

void RunsTableView::mustNotSelectGroup() {
  QMessageBox::critical(this, "Group Selected",
                        "To delete rows you should not deselect any groups.");
}

void RunsTableView::mustSelectGroupOrRow() {
  QMessageBox::critical(
      this, "No Group Or Row Selected",
      "You must select a group or a row to perform this action.");
}

void RunsTableView::onFilterChanged(QString const &filter) {
  m_notifyee->notifyFilterChanged(filter.toStdString());
}

void RunsTableView::onInstrumentChanged(int index) {
  UNUSED_ARG(index);
  m_notifyee->notifyInstrumentChanged();
}

std::string RunsTableView::getInstrumentName() const {
  return m_ui.instrumentSelector->currentText().toStdString();
}

void RunsTableView::setInstrumentName(std::string const &instrumentName) {
  setSelected(*m_ui.instrumentSelector, instrumentName);
}

void RunsTableView::resetFilterBox() { m_ui.filterBox->clear(); }

void RunsTableView::showAlgorithmPropertyHintsInOptionsColumn() {
  auto constexpr optionsColumn = 8;
  m_jobs->setHintsForColumn(
      optionsColumn,
      Mantid::Kernel::make_unique<
          MantidQt::MantidWidgets::AlgorithmHintStrategy>(
          "ReflectometryReductionOneAuto",
          std::vector<std::string>{
              "ThetaIn", "ThetaOut", "InputWorkspace", "OutputWorkspace",
              "OutputWorkspaceBinned", "OutputWorkspaceWavelength",
              "FirstTransmissionRun", "SecondTransmissionRun",
              "MomentumTransferMin", "MomentumTransferMax",
              "MomentumTransferStep", "ScaleFactor"}));
}

void RunsTableView::setJobsTableEnabled(bool enabled) {
  static const auto editTriggers = m_jobs->editTriggers();

  if (enabled)
    m_jobs->setEditTriggers(editTriggers);
  else
    m_jobs->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void RunsTableView::setActionEnabled(Action action, bool enable) {
  m_actions[action]->setEnabled(enable);
}

void RunsTableView::setInstrumentSelectorEnabled(bool enable) {
  m_ui.instrumentSelector->setEnabled(enable);
}

void RunsTableView::setProcessButtonEnabled(bool enable) {
  m_ui.processButton->setEnabled(enable);
}

/**
 * Returns a toolbar item/action
 * @param action
 * @param icon For Qt4 The IconPath i.e. Mantidplot. In Qt5 uses it to get the
 * Icon from the mantidqt.icons library.
 * @param description
 * @return QAction*
 */
QAction *RunsTableView::addToolbarItem(Action action, std::string const &icon,
                                       std::string const &description) {
  QIcon qIcon;
  if (icon == "") {
    qIcon = QIcon(QString::fromStdString(""));
  } else {
    qIcon = MantidQt::Icons::getIcon(QString::fromStdString(icon));
  }

  m_actions[action] =
      m_ui.toolBar->addAction(qIcon, QString::fromStdString(description));
  return m_actions[action];
}

void RunsTableView::addToolbarActions() {
  connect(addToolbarItem(Action::Process, "mdi.sigma", "Process selected runs"),
          SIGNAL(triggered(bool)), this, SLOT(onProcessPressed(bool)));

  connect(
      addToolbarItem(Action::Pause, "mdi.pause", "Pause processing of runs"),
      SIGNAL(triggered(bool)), this, SLOT(onPausePressed(bool)));

  connect(addToolbarItem(Action::Expand, "mdi.expand-all", "Expand all groups"),
          SIGNAL(triggered(bool)), this, SLOT(onExpandAllGroupsPressed(bool)));

  connect(addToolbarItem(Action::Collapse, "mdi.collapse-all",
                         "Collapse all groups"),
          SIGNAL(triggered(bool)), this,
          SLOT(onCollapseAllGroupsPressed(bool)));

  connect(addToolbarItem(Action::PlotSelected, "mdi.chart-line",
                         "Plot selected rows as graphs"),
          SIGNAL(triggered(bool)), this, SLOT(onPlotSelectedPressed(bool)));

  connect(addToolbarItem(Action::PlotSelectedStitchedOutput,
                         "mdi.chart-areaspline",
                         "Plot selected rows with stitched outputs as graphs"),
          SIGNAL(triggered(bool)), this,
          SLOT(onPlotSelectedStitchedOutputPressed(bool)));

  connect(addToolbarItem(Action::InsertRow, "mdi.table-row-plus-after",
                         "Insert row into selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertRowPressed(bool)));

  connect(addToolbarItem(Action::DeleteRow, "mdi.table-row-remove",
                         "Delete all selected rows"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteRowPressed(bool)));

  connect(addToolbarItem(Action::InsertGroup, "mdi.table-plus",
                         "Insert group after first selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertGroupPressed(bool)));

  connect(addToolbarItem(Action::DeleteGroup, "mdi.table-remove",
                         "Delete all selected groups"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteGroupPressed(bool)));

  connect(addToolbarItem(Action::Copy, "mdi.content-copy",
                         "Copy the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onCopyPressed(bool)));

  connect(addToolbarItem(Action::Paste, "mdi.content-paste",
                         "Paste over the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onPastePressed(bool)));

  connect(addToolbarItem(Action::Cut, "mdi.content-cut",
                         "Cut the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onCutPressed(bool)));
}

MantidQt::MantidWidgets::Batch::IJobTreeView &RunsTableView::jobs() {
  return *m_jobs;
}

void RunsTableView::subscribe(RunsTableViewSubscriber *notifyee) {
  m_notifyee = notifyee;
  m_jobs->subscribe(*notifyee);
  connect(m_ui.processButton, SIGNAL(clicked(bool)), this,
          SLOT(onProcessPressed(bool)));
}

void RunsTableView::setProgress(int value) {
  m_ui.progressBar->setValue(value);
}

void RunsTableView::onExpandAllGroupsPressed(bool) {
  m_notifyee->notifyExpandAllRequested();
}

void RunsTableView::onCollapseAllGroupsPressed(bool) {
  m_notifyee->notifyCollapseAllRequested();
}

void RunsTableView::onProcessPressed(bool) {
  m_notifyee->notifyReductionResumed();
}

void RunsTableView::onPausePressed(bool) {
  m_notifyee->notifyReductionPaused();
}

void RunsTableView::onInsertRowPressed(bool) {
  m_notifyee->notifyInsertRowRequested();
}

void RunsTableView::onInsertGroupPressed(bool) {
  m_notifyee->notifyInsertGroupRequested();
}

void RunsTableView::onDeleteRowPressed(bool) {
  m_notifyee->notifyDeleteRowRequested();
}

void RunsTableView::onDeleteGroupPressed(bool) {
  m_notifyee->notifyDeleteGroupRequested();
}

void RunsTableView::onCopyPressed(bool) {
  m_notifyee->notifyCopyRowsRequested();
}

void RunsTableView::onCutPressed(bool) { m_notifyee->notifyCutRowsRequested(); }

void RunsTableView::onPastePressed(bool) {
  m_notifyee->notifyPasteRowsRequested();
}

void RunsTableView::onPlotSelectedPressed(bool) {
  m_notifyee->notifyPlotSelectedPressed();
}

void RunsTableView::onPlotSelectedStitchedOutputPressed(bool) {
  m_notifyee->notifyPlotSelectedStitchedOutputPressed();
}

/** Set a combo box to the given value
 */
void RunsTableView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

RunsTableViewFactory::RunsTableViewFactory(
    std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

RunsTableView *RunsTableViewFactory::operator()() const {
  return new RunsTableView(m_instruments, defaultInstrumentFromConfig());
}

int RunsTableViewFactory::indexOfElseFirst(
    std::string const &instrument) const {
  return indexOf(m_instruments,
                 [&instrument](std::string const &inst) {
                   return instrument == inst;
                 })
      .get_value_or(0);
}

int RunsTableViewFactory::defaultInstrumentFromConfig() const {
  return indexOfElseFirst(Mantid::Kernel::ConfigService::Instance().getString(
      "default.instrument"));
}
} // namespace CustomInterfaces
} // namespace MantidQt
