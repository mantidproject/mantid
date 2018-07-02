#include "QtReflMainWindowView.h"
#include "MantidKernel/make_unique.h"
#include "QtReflBatchView.h"
#include "Reduction/Slicing.h"

#include <QMessageBox>
#include <QToolButton>

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(QtReflMainWindowView)

QtReflMainWindowView::QtReflMainWindowView(QWidget *parent)
    : UserSubWindow(parent) {}

IReflBatchView *QtReflMainWindowView::newBatch() {
  auto index = m_ui.mainTabs->count();
  auto *newTab = new QtReflBatchView(this);
  m_ui.mainTabs->addTab(newTab, QString("Batch ") + QString::number(index));
  m_batchViews.emplace_back(newTab);
  return newTab;
}

void QtReflMainWindowView::removeBatch(int batchIndex) {
  m_batchViews.erase(m_batchViews.begin() + batchIndex);
  m_ui.mainTabs->removeTab(batchIndex);
}

std::vector<IReflBatchView *> QtReflMainWindowView::batches() const {
  return m_batchViews;
}

/**
Initialise the Interface
*/
void QtReflMainWindowView::initLayout() {
  m_ui.setupUi(this);
  connect(m_ui.helpButton, SIGNAL(clicked()), this, SLOT(helpPressed()));
  connect(m_ui.mainTabs, SIGNAL(tabCloseRequested(int)), this,
          SLOT(onTabCloseRequested(int)));
  connect(m_ui.newBatch, SIGNAL(triggered(bool)), this,
          SLOT(onNewBatchRequested(bool)));

  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});

  auto defaultSlicing = Slicing();
  auto thetaTolerance = 0.01;
  auto makeWorkspaceNames = WorkspaceNamesFactory(defaultSlicing);
  auto makeBatchPresenter = RunsTablePresenterFactory(
      instruments, thetaTolerance, makeWorkspaceNames);
  auto defaultInstrumentIndex = 0; // TODO: Look this up properly;
  auto searcher = boost::shared_ptr<IReflSearcher>();

  auto makeRunsPresenter = RunsPresenterFactory(
      std::move(makeBatchPresenter), std::move(makeWorkspaceNames),
      thetaTolerance, instruments, defaultInstrumentIndex, searcher);

  auto makeEventPresenter = EventPresenterFactory();
  auto makeSettingsPresenter = SettingsPresenterFactory();
  auto makeSaveSettingsPresenter = SavePresenterFactory();

  auto makeExperimentPresenter = ExperimentPresenterFactory();

  auto makeReflBatchPresenter = ReflBatchPresenterFactory(
      std::move(makeRunsPresenter), std::move(makeEventPresenter),
      std::move(makeExperimentPresenter), std::move(makeSettingsPresenter),
      std::move(makeSaveSettingsPresenter));

  // Create the presenter
  m_presenter =
      ReflMainWindowPresenter(this, std::move(makeReflBatchPresenter));
  subscribe(&m_presenter.get());

  m_presenter.get().notifyNewBatchRequested();
  m_presenter.get().notifyNewBatchRequested();
}

void QtReflMainWindowView::onTabCloseRequested(int tabIndex) {
  m_ui.mainTabs->removeTab(tabIndex);
}

void QtReflMainWindowView::onNewBatchRequested(bool) {
  m_notifyee->notifyNewBatchRequested();
}

void QtReflMainWindowView::subscribe(ReflMainWindowSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtReflMainWindowView::helpPressed() { m_notifyee->notifyHelpPressed(); }

/**
Runs python code
* @param pythonCode : [input] The code to run
* @return : Result of the execution
*/
std::string
QtReflMainWindowView::runPythonAlgorithm(const std::string &pythonCode) {

  QString output = runPythonCode(QString::fromStdString(pythonCode), false);
  return output.toStdString();
}

/**
Handles attempt to close main window
* @param event : [input] The close event
*/
void QtReflMainWindowView::closeEvent(QCloseEvent *event) {
  // Close only if reduction has been paused
  if (!m_presenter.get().isProcessing()) {
    event->accept();
  } else {
    event->ignore();
  }
}
}
}
