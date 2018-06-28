#include "RunsTableView.h"
#include "../../IndexOf.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>

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

QAction *RunsTableView::addToolbarItem(std::string const &iconPath,
                                   std::string const &description) {
  return m_ui.toolBar->addAction(QIcon(QString::fromStdString(iconPath)),
                                 QString::fromStdString(description));
}

void RunsTableView::addToolbarActions() {
  connect(addToolbarItem("://stat_rows.png", "Process selected runs."),
          SIGNAL(triggered(bool)), this, SLOT(onProcessPressed(bool)));
  connect(addToolbarItem("://pause.png", "Pause processing of runs."),
          SIGNAL(triggered(bool)), this, SLOT(onPausePressed(bool)));
  connect(addToolbarItem("://insert_row.png", "Insert row into selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertRowPressed(bool)));
  connect(addToolbarItem("://insert_group.png",
                         "Insert group after first selected"),
          SIGNAL(triggered(bool)), this, SLOT(onInsertGroupPressed(bool)));
  connect(addToolbarItem("://delete_row.png", "Delete all selected rows"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteRowPressed(bool)));
  connect(addToolbarItem("://delete_group.png", "Delete all selected groups"),
          SIGNAL(triggered(bool)), this, SLOT(onDeleteGroupPressed(bool)));
  connect(addToolbarItem("://copy.png", "Copy the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onCopyPressed(bool)));
  connect(addToolbarItem("://paste.png", "Paste over the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onPastePressed(bool)));
  connect(addToolbarItem("://cut.png", "Cut the current selection"),
          SIGNAL(triggered(bool)), this, SLOT(onCutPressed(bool)));
  connect(addToolbarItem("://expand_all.png", "Expand all groups"),
          SIGNAL(triggered(bool)), this, SLOT(onExpandAllGroupsPressed(bool)));
  connect(addToolbarItem("://collapse_all.png", "Collapse all groups"),
          SIGNAL(triggered(bool)), this,
          SLOT(onCollapseAllGroupsPressed(bool)));
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

void RunsTableView::setProgress(int value) { m_ui.progressBar->setValue(value); }

void RunsTableView::onExpandAllGroupsPressed(bool) {
  m_notifyee->notifyExpandAllRequested();
}

void RunsTableView::onCollapseAllGroupsPressed(bool) {
  m_notifyee->notifyCollapseAllRequested();
}

void RunsTableView::onProcessPressed(bool) { m_notifyee->notifyProcessRequested(); }
void RunsTableView::onPausePressed(bool) { m_notifyee->notifyPauseRequested(); }

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

void RunsTableView::onCopyPressed(bool) { m_notifyee->notifyCopyRowsRequested(); }

void RunsTableView::onCutPressed(bool) { m_notifyee->notifyCutRowsRequested(); }

void RunsTableView::onPastePressed(bool) { m_notifyee->notifyPasteRowsRequested(); }

RunsTableViewFactory::RunsTableViewFactory(std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

RunsTableView *RunsTableViewFactory::operator()() const {
  return new RunsTableView(m_instruments, defaultInstrumentFromConfig());
}

int RunsTableViewFactory::indexOfElseFirst(std::string const &instrument) const {
  return indexOf(m_instruments, [&instrument](std::string const &inst) {
    return instrument == inst;
  }).get_value_or(0);
}

int RunsTableViewFactory::defaultInstrumentFromConfig() const {
  return indexOfElseFirst(Mantid::Kernel::ConfigService::Instance().getString(
      "default.instrument"));
}
}
}
