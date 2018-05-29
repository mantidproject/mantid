#include "BatchView.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ConfigService.h"

namespace MantidQt {
namespace CustomInterfaces {
BatchView::BatchView(std::vector<std::string> const &instruments,
                     int defaultInstrumentIndex)
    : m_jobs(), m_instruments(instruments) {
  m_ui.setupUi(this);
  m_jobs =
      Mantid::Kernel::make_unique<MantidQt::MantidWidgets::Batch::JobTreeView>(
          QStringList({"Run(s)", "Angle", "First Transmission Run",
                       "Second Transmission Run", "Q min", "Q max", "dQ/Q",
                       "Scale", "Options"}),
          MantidQt::MantidWidgets::Batch::Cell(""), this);
  m_ui.mainLayout->insertWidget(1, m_jobs.get());
  addToolbarActions();
  m_jobs->addActions(m_ui.toolBar->actions());

  for (auto &&instrument : m_instruments)
    m_ui.instrumentSelector->addItem(QString::fromStdString(instrument));
  m_ui.instrumentSelector->setCurrentIndex(defaultInstrumentIndex);
}

QAction *BatchView::addToolbarItem(std::string const &iconPath,
                                   std::string const &description) {
  return m_ui.toolBar->addAction(QIcon(QString::fromStdString(iconPath)),
                                 QString::fromStdString(description));
}

void BatchView::addToolbarActions() {
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

MantidQt::MantidWidgets::Batch::IJobTreeView &BatchView::jobs() {
  return *m_jobs;
}

void BatchView::subscribe(BatchViewSubscriber *notifyee) {
  m_notifyee = notifyee;
  m_jobs->subscribe(*notifyee);
  connect(m_ui.processButton, SIGNAL(clicked(bool)), this,
          SLOT(onProcessPressed(bool)));
}

void BatchView::setProgress(int value) { m_ui.progressBar->setValue(value); }

void BatchView::onExpandAllGroupsPressed(bool) {
  m_notifyee->notifyExpandAllRequested();
}

void BatchView::onCollapseAllGroupsPressed(bool) {
  m_notifyee->notifyCollapseAllRequested();
}

void BatchView::onProcessPressed(bool) { m_notifyee->notifyProcessRequested(); }
void BatchView::onPausePressed(bool) { m_notifyee->notifyPauseRequested(); }

void BatchView::onInsertRowPressed(bool) {
  m_notifyee->notifyInsertRowRequested();
}

void BatchView::onInsertGroupPressed(bool) {
  m_notifyee->notifyInsertGroupRequested();
}

void BatchView::onDeleteRowPressed(bool) {
  m_notifyee->notifyDeleteRowRequested();
}

void BatchView::onDeleteGroupPressed(bool) {
  m_notifyee->notifyDeleteGroupRequested();
}

void BatchView::onCopyPressed(bool) { m_notifyee->notifyCopyRowsRequested(); }

void BatchView::onCutPressed(bool) { m_notifyee->notifyCutRowsRequested(); }

void BatchView::onPastePressed(bool) { m_notifyee->notifyPasteRowsRequested(); }

BatchViewFactory::BatchViewFactory(std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

BatchView *BatchViewFactory::operator()() const {
  return new BatchView(m_instruments, defaultInstrumentFromConfig());
}

int BatchViewFactory::indexOfElseFirst(std::string const &instrument) const {
  auto it = std::find(m_instruments.cbegin(), m_instruments.cend(), instrument);
  if (it != m_instruments.cend())
    return static_cast<int>(std::distance(m_instruments.cbegin(), it));
  else
    return 0;
}

int BatchViewFactory::defaultInstrumentFromConfig() const {
  return indexOfElseFirst(Mantid::Kernel::ConfigService::Instance().getString(
      "default.instrument"));
}
}
}
