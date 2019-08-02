// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QMainWindowView.h"
#include "Common/IndexOf.h"
#include "GUI/Batch/QBatchView.h"
#include "GUI/Common/Plotter.h"
#include <QMessageBox>
#include <QToolButton>

namespace MantidQt {
namespace CustomInterfaces {

namespace {

int getDefaultInstrumentIndex(std::vector<std::string> &instruments) {
  auto instrumentName =
      Mantid::Kernel::ConfigService::Instance().getString("default.instrument");
  auto result = indexOfValue(instruments, instrumentName);
  if (result)
    return *result;
  // If not found, use first instrument
  return 0;
}
} // namespace

DECLARE_SUBWINDOW(QMainWindowView)

QMainWindowView::QMainWindowView(QWidget *parent)
    : UserSubWindow(parent), m_notifyee(nullptr) {}

IBatchView *QMainWindowView::newBatch() {
  auto index = m_ui.mainTabs->count();
  auto *newTab = new QBatchView(this);
  m_ui.mainTabs->addTab(newTab, QString("Batch ") + QString::number(index));
  m_batchViews.emplace_back(newTab);
  return newTab;
}

void QMainWindowView::removeBatch(int batchIndex) {
  m_batchViews.erase(m_batchViews.begin() + batchIndex);
  m_ui.mainTabs->removeTab(batchIndex);
  if (m_ui.mainTabs->count() == 0) {
    m_notifyee->notifyNewBatchRequested();
  }
}

std::vector<IBatchView *> QMainWindowView::batches() const {
  return m_batchViews;
}

/**
Initialise the Interface
*/
void QMainWindowView::initLayout() {
  m_ui.setupUi(this);
  // Until this is implemented we should hide this action
  m_ui.loadBatch->setEnabled(false);
  m_ui.loadBatch->setVisible(false);

  connect(m_ui.helpButton, SIGNAL(clicked()), this, SLOT(helpPressed()));
  connect(m_ui.mainTabs, SIGNAL(tabCloseRequested(int)), this,
          SLOT(onTabCloseRequested(int)));
  connect(m_ui.newBatch, SIGNAL(triggered(bool)), this,
          SLOT(onNewBatchRequested(bool)));

  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});

  auto thetaTolerance = 0.01;
  auto pythonRunner = this;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  Plotter plotter(pythonRunner);
#else
  Plotter plotter;
#endif
  auto makeRunsTablePresenter = RunsTablePresenterFactory(
      instruments, thetaTolerance, std::move(plotter));

  auto defaultInstrumentIndex = getDefaultInstrumentIndex(instruments);
  auto messageHandler = this;
  auto makeRunsPresenter = RunsPresenterFactory(
      std::move(makeRunsTablePresenter), thetaTolerance, instruments,
      defaultInstrumentIndex, messageHandler, pythonRunner);

  auto makeEventPresenter = EventPresenterFactory();
  auto makeSaveSettingsPresenter = SavePresenterFactory();
  auto makeExperimentPresenter = ExperimentPresenterFactory(thetaTolerance);
  auto makeInstrumentPresenter = InstrumentPresenterFactory();

  auto makeBatchPresenter = BatchPresenterFactory(
      std::move(makeRunsPresenter), std::move(makeEventPresenter),
      std::move(makeExperimentPresenter), std::move(makeInstrumentPresenter),
      std::move(makeSaveSettingsPresenter));

  // Create the presenter
  m_presenter = std::make_unique<MainWindowPresenter>(
      this, messageHandler, std::move(makeBatchPresenter));

  m_notifyee->notifyNewBatchRequested();
  m_notifyee->notifyNewBatchRequested();
}

void QMainWindowView::onTabCloseRequested(int tabIndex) {
  m_notifyee->notifyCloseBatchRequested(tabIndex);
}

void QMainWindowView::onNewBatchRequested(bool) {
  m_notifyee->notifyNewBatchRequested();
}

void QMainWindowView::subscribe(MainWindowSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QMainWindowView::helpPressed() { m_notifyee->notifyHelpPressed(); }

/**
Runs python code
* @param pythonCode : [input] The code to run
* @return : Result of the execution
*/
std::string QMainWindowView::runPythonAlgorithm(const std::string &pythonCode) {

  QString output = runPythonCode(QString::fromStdString(pythonCode), false);
  return output.toStdString();
}

/**
Handles attempt to close main window
* @param event : [input] The close event
*/
void QMainWindowView::closeEvent(QCloseEvent *event) {
  // Close only if reduction has been paused
  if (!m_presenter->isAnyBatchProcessing() ||
      m_presenter->isAnyBatchAutoreducing()) {
    event->accept();
  } else {
    event->ignore();
  }
}

void QMainWindowView::giveUserCritical(const std::string &prompt,
                                       const std::string &title) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void QMainWindowView::giveUserInfo(const std::string &prompt,
                                   const std::string &title) {
  QMessageBox::information(this, QString::fromStdString(title),
                           QString::fromStdString(prompt), QMessageBox::Ok,
                           QMessageBox::Ok);
}

bool QMainWindowView::askUserYesNo(const std::string &prompt,
                                   const std::string &title) {
  auto reply = QMessageBox::question(this, QString::fromStdString(title),
                                     QString::fromStdString(prompt),
                                     QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes)
    return true;

  return false;
}
} // namespace CustomInterfaces
} // namespace MantidQt
