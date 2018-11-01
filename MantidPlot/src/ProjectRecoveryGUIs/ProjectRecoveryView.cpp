// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ProjectRecoveryView.h"
#include "ApplicationWindow.h"
#include "MantidKernel/UsageService.h"
#include "MultiTabScriptInterpreter.h"
#include "Script.h"
#include "ScriptFileInterpreter.h"
#include "ScriptingWindow.h"
#include "ui_ProjectRecoveryWidget.h"
#include <boost/smart_ptr/make_shared.hpp>

ProjectRecoveryView::ProjectRecoveryView(QWidget *parent,
                                         ProjectRecoveryPresenter *presenter)
    : QDialog(parent), ui(new Ui::ProjectRecoveryWidget),
      m_presenter(presenter) {
  ui->setupUi(this);
  ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);
  ui->progressBar->setMinimum(0);
  // Set the table information
  addDataToTable(ui);
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "ProjectRecoveryWindow", true);
}

ProjectRecoveryView::~ProjectRecoveryView() { delete ui; }

void ProjectRecoveryView::addDataToTable(Ui::ProjectRecoveryWidget *ui) {
  QStringList row = m_presenter->getRow(0);
  ui->tableWidget->setItem(0, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(0, 1, new QTableWidgetItem(row[1]));
}

void ProjectRecoveryView::onClickLastCheckpoint() {
  // Recover last checkpoint
  m_presenter->recoverLast(boost::make_shared<QDialog>(this));
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryWindow->RecoverLastCheckpoint", false);
}

void ProjectRecoveryView::onClickOpenLastInScriptWindow() {
  // Open checkpoint in script window
  m_presenter->openLastInEditor();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryWindow->OpenInScriptWindow", false);
}

void ProjectRecoveryView::onClickStartMantidNormally() {
  // Start save and close this, clear checkpoint that was offered for load
  m_presenter->startMantidNormally();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryWindow->StartMantidNormally", false);
}

void ProjectRecoveryView::reject() {
  // Do the same as startMantidNormally
  m_presenter->startMantidNormally();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryWindow->StartMantidNormally", false);
}

void ProjectRecoveryView::updateProgressBar(int newValue, bool err) {
  if (!err) {
    ui->progressBar->setValue(newValue);
  }
}

void ProjectRecoveryView::setProgressBarMaximum(int newValue) {
  ui->progressBar->setMaximum(newValue);
}

void ProjectRecoveryView::connectProgressBar() {
  connect(&m_presenter->m_mainWindow->getScriptWindowHandle()
               ->getCurrentScriptRunner(),
          SIGNAL(currentLineChanged(int, bool)), this,
          SLOT(updateProgressBar(int, bool)));
}