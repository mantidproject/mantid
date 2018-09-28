#include "ProjectRecoveryView.h"
#include "MantidKernel/UsageService.h"
#include "ui_ProjectRecoveryWidget.h"

ProjectRecoveryView::ProjectRecoveryView(QWidget *parent,
                                         ProjectRecoveryPresenter *presenter)
    : QDialog(parent), ui(new Ui::ProjectRecoveryWidget),
      m_presenter(presenter) {

  ui->setupUi(this);
  ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);
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
  // Do nothing just absorb request
  m_presenter->startMantidNormally();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryWindow->StartMantidNormally", false);
}