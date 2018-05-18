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
          QStringList({"Run(s)", "Angle", "Transmission Run(s)", "Q min",
                       "Q max", "dQ/Q", "Scale", "Options"}),
          MantidQt::MantidWidgets::Batch::Cell(""), this);
  m_ui.mainLayout->insertWidget(1, m_jobs.get());
  auto *action =
      m_ui.toolBar->addAction(QIcon("://expand_all.png"), "Expand All Groups");
  connect(action, SIGNAL(triggered(bool)), this,
          SLOT(onExpandAllGroupsPressed(bool)));

  for (auto &&instrument : m_instruments)
    m_ui.instrumentSelector->addItem(QString::fromStdString(instrument));
  m_ui.instrumentSelector->setCurrentIndex(defaultInstrumentIndex);
}

MantidQt::MantidWidgets::Batch::IJobTreeView &BatchView::jobs() {
  return *m_jobs;
}

void BatchView::expandAllGroups() { m_jobs->expandAll(); }

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

void BatchView::onProcessPressed(bool) { m_notifyee->notifyProcessRequested(); }

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
