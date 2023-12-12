// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtMainWindowView.h"
#include "Common/IndexOf.h"
#include "GUI/Batch/BatchPresenterFactory.h"
#include "GUI/Batch/QtBatchView.h"
#include "GUI/Common/Decoder.h"
#include "GUI/Common/Encoder.h"
#include "GUI/Common/Plotter.h"
#include "GUI/Options/OptionsDialogModel.h"
#include "GUI/Options/OptionsDialogPresenter.h"
#include "GUI/Options/QtOptionsDialogView.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/QtJSONUtils.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolButton>

#include <fstream>

using Mantid::API::FileFinder;

namespace MantidQt {

using MantidWidgets::SlitCalculator;

namespace CustomInterfaces::ISISReflectometry {

// Do not change the last argument as you will break backwards compatibility
// with project save it should be the same as one of the tags in the decoder.
DECLARE_SUBWINDOW_AND_CODERS(QtMainWindowView, Encoder, Decoder, "ISIS Reflectometry")

QtMainWindowView::QtMainWindowView(QWidget *parent) : UserSubWindow(parent), m_notifyee(nullptr), m_batchIndex(1) {}

QtMainWindowView::~QtMainWindowView() = default;

IBatchView *QtMainWindowView::newBatch() {
  auto *newTab = new QtBatchView(this);
  m_ui.mainTabs->addTab(newTab, QString("Batch ") + QString::number(m_batchIndex));
  m_batchViews.emplace_back(newTab);
  ++m_batchIndex;
  return newTab;
}

void QtMainWindowView::removeBatch(int batchIndex) {
  m_batchViews.erase(m_batchViews.begin() + batchIndex);
  auto *batchToRemove = m_ui.mainTabs->widget(batchIndex);
  m_ui.mainTabs->removeTab(batchIndex);
  // Calling removeTab doesn't deallocate the memory for the QtBatchView, so we must do that here
  delete batchToRemove;
  if (m_ui.mainTabs->count() == 0) {
    m_notifyee->notifyNewBatchRequested();
  }
}

std::vector<IBatchView *> QtMainWindowView::batches() const { return m_batchViews; }

/**
Initialise the Interface
*/
void QtMainWindowView::initLayout() {
  m_ui.setupUi(this);

  m_ui.mainTabs->setUsesScrollButtons(true);

  connect(m_ui.helpButton, SIGNAL(clicked()), this, SLOT(helpPressed()));
  connect(m_ui.mainTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));
  connect(m_ui.newBatch, SIGNAL(triggered(bool)), this, SLOT(onNewBatchRequested(bool)));
  connect(m_ui.loadBatch, SIGNAL(triggered(bool)), this, SLOT(onLoadBatchRequested(bool)));
  connect(m_ui.saveBatch, SIGNAL(triggered(bool)), this, SLOT(onSaveBatchRequested(bool)));
  connect(m_ui.showOptions, SIGNAL(triggered(bool)), this, SLOT(onShowOptionsRequested(bool)));
  connect(m_ui.showSlitCalculator, SIGNAL(triggered(bool)), this, SLOT(onShowSlitCalculatorRequested(bool)));

  auto instruments = std::vector<std::string>({{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});

  auto thetaTolerance = 0.01;
  Plotter plotter;

  auto makeRunsTablePresenter = RunsTablePresenterFactory(instruments, thetaTolerance, std::move(plotter));

  auto messageHandler = this;
  auto fileHandler = this;
  auto makeRunsPresenter =
      RunsPresenterFactory(std::move(makeRunsTablePresenter), thetaTolerance, instruments, messageHandler, fileHandler);

  auto makeEventPresenter = EventPresenterFactory();
  auto makeSaveSettingsPresenter = SavePresenterFactory(fileHandler);
  auto makeExperimentPresenter = ExperimentPresenterFactory(fileHandler, thetaTolerance);
  auto makeInstrumentPresenter = InstrumentPresenterFactory(fileHandler, messageHandler);
  auto makePreviewPresenter = PreviewPresenterFactory();

  auto makeBatchPresenter = std::make_unique<BatchPresenterFactory>(
      std::move(makeRunsPresenter), makeEventPresenter, makeExperimentPresenter, makeInstrumentPresenter,
      makePreviewPresenter, makeSaveSettingsPresenter, messageHandler);

  // Create the presenter
  auto slitCalculator = std::make_unique<SlitCalculator>(this);
  m_optionsDialogView = std::make_unique<QtOptionsDialogView>(this);
  auto optionsDialogPresenter =
      std::make_unique<OptionsDialogPresenter>(m_optionsDialogView.get(), std::make_unique<OptionsDialogModel>());
  m_presenter = std::make_unique<MainWindowPresenter>(this, messageHandler, fileHandler, std::make_unique<Encoder>(),
                                                      std::make_unique<Decoder>(), std::move(slitCalculator),
                                                      std::move(optionsDialogPresenter), std::move(makeBatchPresenter));

  m_notifyee->notifyNewBatchRequested();
  m_notifyee->notifyNewBatchRequested();
}

void QtMainWindowView::onTabCloseRequested(int tabIndex) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "CloseBatch"}, false);
  m_notifyee->notifyCloseBatchRequested(tabIndex);
}

void QtMainWindowView::onNewBatchRequested(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "NewBatch"}, false);
  m_notifyee->notifyNewBatchRequested();
}

void QtMainWindowView::onLoadBatchRequested(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "LoadBatch"}, false);
  m_notifyee->notifyLoadBatchRequested(m_ui.mainTabs->currentIndex());
}

void QtMainWindowView::onSaveBatchRequested(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "SaveBatch"}, false);
  m_notifyee->notifySaveBatchRequested(m_ui.mainTabs->currentIndex());
}

void QtMainWindowView::onShowOptionsRequested(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "ShowOptions"}, false);
  m_notifyee->notifyShowOptionsRequested();
}

void QtMainWindowView::onShowSlitCalculatorRequested(bool) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "ShowSlitCalculator"}, false);
  m_notifyee->notifyShowSlitCalculatorRequested();
}

void QtMainWindowView::subscribe(MainWindowSubscriber *notifyee) { m_notifyee = notifyee; }

void QtMainWindowView::helpPressed() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "MainWindow", "ShowHelp"}, false);
  m_notifyee->notifyHelpPressed();
}

/**
Runs python code
* @param pythonCode : [input] The code to run
* @return : Result of the execution
*/
std::string QtMainWindowView::runPythonAlgorithm(const std::string &pythonCode) {

  QString output = runPythonCode(QString::fromStdString(pythonCode), false);
  return output.toStdString();
}

/**
Handles attempt to close main window
* @param event : [input] The close event
*/
void QtMainWindowView::closeEvent(QCloseEvent *event) {
  m_closeEvent = event;
  m_notifyee->notifyCloseEvent();
}

void QtMainWindowView::acceptCloseEvent() { m_closeEvent->accept(); }

void QtMainWindowView::ignoreCloseEvent() { m_closeEvent->ignore(); }

void QtMainWindowView::giveUserCritical(const std::string &prompt, const std::string &title) {
  QMessageBox::critical(this, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

void QtMainWindowView::giveUserWarning(const std::string &prompt, const std::string &title) {
  QMessageBox::warning(this, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void QtMainWindowView::giveUserInfo(const std::string &prompt, const std::string &title) {
  QMessageBox::information(this, QString::fromStdString(title), QString::fromStdString(prompt), QMessageBox::Ok,
                           QMessageBox::Ok);
}

bool QtMainWindowView::askUserOkCancel(const std::string &prompt, const std::string &title) {
  auto reply = QMessageBox::question(this, QString::fromStdString(title), QString::fromStdString(prompt),
                                     QMessageBox::Ok | QMessageBox::Cancel);
  return (reply == QMessageBox::Ok);
}

std::string QtMainWindowView::askUserForLoadFileName(std::string const &filter) {
  auto filterQString = QString::fromStdString(filter);
  auto filename = QFileDialog::getOpenFileName(nullptr, QString(), QString(), filterQString, nullptr,
                                               QFileDialog::DontResolveSymlinks);
  return filename.toStdString();
}

std::string QtMainWindowView::askUserForSaveFileName(std::string const &filter) {
  auto filterQString = QString::fromStdString(filter);
  auto filename = QFileDialog::getSaveFileName(nullptr, QString(), QString(), filterQString, nullptr,
                                               QFileDialog::DontResolveSymlinks);
  return filename.toStdString();
}

void QtMainWindowView::disableSaveAndLoadBatch() {
  m_ui.saveBatch->setEnabled(false);
  m_ui.loadBatch->setEnabled(false);
}

void QtMainWindowView::enableSaveAndLoadBatch() {
  m_ui.saveBatch->setEnabled(true);
  m_ui.loadBatch->setEnabled(true);
}

void QtMainWindowView::saveJSONToFile(std::string const &filename, QMap<QString, QVariant> const &map) {
  auto filenameQString = QString::fromStdString(filename);
  MantidQt::API::saveJSONToFile(filenameQString, map);
}

QMap<QString, QVariant> QtMainWindowView::loadJSONFromFile(std::string const &filename) {
  return MantidQt::API::loadJSONFromFile(QString::fromStdString(filename));
}

void QtMainWindowView::saveCSVToFile(const std::string &filename, const std::string &content) const {
  std::ofstream outFile(filename, std::ios::trunc);
  if (!outFile) {
    throw std::runtime_error("Saving to " + filename + "failed. Please try again.");
  }
  outFile << content;
  outFile.close();
}

bool QtMainWindowView::fileExists(std::string const &filepath) const {
  if (filepath.empty())
    return false;

  try {
    auto pocoPath = Poco::Path().parseDirectory(filepath);
    auto pocoFile = Poco::File(pocoPath);
    if (!pocoFile.exists())
      return false;
  } catch (Poco::PathSyntaxException &) {
    return false;
  }
  return true;
}

std::string QtMainWindowView::getFullFilePath(const std::string &filename) const {
  return FileFinder::Instance().getFullPath(filename);
}

} // namespace CustomInterfaces::ISISReflectometry
} // namespace MantidQt
