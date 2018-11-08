// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ProjectRecoveryView.h"
#include "ApplicationWindow.h"
#include "MantidKernel/UsageService.h"
#include "Script.h"
#include "ScriptingWindow.h"

ProjectRecoveryView::ProjectRecoveryView(QWidget *parent,
                                         ProjectRecoveryPresenter *presenter)
    : QDialog(parent), m_ui(std::make_unique<Ui::ProjectRecoveryWidget>()),
      m_presenter(presenter) {
  m_ui->setupUi(this);
  m_ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  m_ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);
  m_ui->progressBar->setMinimum(0);
  // Set the table information
  addDataToTable();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "ProjectRecoveryWindow", true);
}

void ProjectRecoveryView::addDataToTable() {
  const QStringList row = m_presenter->getRow(0);
  m_ui->tableWidget->setItem(0, 0, new QTableWidgetItem(row[0]));
  m_ui->tableWidget->setItem(0, 1, new QTableWidgetItem(row[1]));
}

void ProjectRecoveryView::onClickLastCheckpoint() {
  // Recover last checkpoint
  m_presenter->recoverLast();
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
    m_ui->progressBar->setValue(newValue);
  }
}

void ProjectRecoveryView::setProgressBarMaximum(int newValue) {
  m_ui->progressBar->setMaximum(newValue);
}

void ProjectRecoveryView::connectProgressBar() {
  connect(&m_presenter->m_mainWindow->getScriptWindowHandle()
               ->getCurrentScriptRunner(),
          SIGNAL(currentLineChanged(int, bool)), this,
          SLOT(updateProgressBar(int, bool)));
}

void ProjectRecoveryView::emitAbortScript() {
  connect(this, SIGNAL(abortProjectRecoveryScript()),
          m_presenter->m_mainWindow->getScriptWindowHandle(),
          SLOT(abortCurrent()));
  emit(abortProjectRecoveryScript());
}

void ProjectRecoveryView::changeStartMantidButton(const QString &string) {
  m_ui->startmantidButton->setText(string);
}