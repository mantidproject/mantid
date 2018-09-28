#include "RecoveryFailureView.h"
#include "ui_RecoveryFailure.h"

#include "MantidKernel/UsageService.h"

RecoveryFailureView::RecoveryFailureView(QWidget *parent,
                                         ProjectRecoveryPresenter *presenter)
    : QDialog(parent), ui(new Ui::RecoveryFailure), m_presenter(presenter) {
  ui->setupUi(this);
  ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::Stretch);
  // Set the table information
  addDataToTable(ui);
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "ProjectRecoveryFailureWindow", true);
}

RecoveryFailureView::~RecoveryFailureView() { delete ui; }

void RecoveryFailureView::addDataToTable(Ui::RecoveryFailure *ui) {
  QStringList row = m_presenter->getRow(0);
  ui->tableWidget->setItem(0, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(0, 1, new QTableWidgetItem(row[1]));
  ui->tableWidget->setItem(0, 2, new QTableWidgetItem(row[2]));
  row = m_presenter->getRow(1);
  ui->tableWidget->setItem(1, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(1, 1, new QTableWidgetItem(row[1]));
  ui->tableWidget->setItem(1, 2, new QTableWidgetItem(row[2]));
  row = m_presenter->getRow(2);
  ui->tableWidget->setItem(2, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(2, 1, new QTableWidgetItem(row[1]));
  ui->tableWidget->setItem(2, 2, new QTableWidgetItem(row[2]));
  row = m_presenter->getRow(3);
  ui->tableWidget->setItem(3, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(3, 1, new QTableWidgetItem(row[1]));
  ui->tableWidget->setItem(3, 2, new QTableWidgetItem(row[2]));
  row = m_presenter->getRow(4);
  ui->tableWidget->setItem(4, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(4, 1, new QTableWidgetItem(row[1]));
  ui->tableWidget->setItem(4, 2, new QTableWidgetItem(row[2]));
}

void RecoveryFailureView::onClickLastCheckpoint() {
  // Recover last checkpoint
  m_presenter->recoverLast();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryFailureWindow->RecoverLastCheckpoint", false);
}

void RecoveryFailureView::onClickSelectedCheckpoint() {
  // Recover Selected
  QList<QTableWidgetItem *> selectedRows = ui->tableWidget->selectedItems();
  if (selectedRows.size() > 0) {
    QString text = selectedRows[0]->text();
    if (text.toStdString() == "") {
      return;
    }
    m_presenter->recoverSelectedCheckpoint(text);
  }
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryFailureWindow->RecoverSelectedCheckpoint",
      false);
}

void RecoveryFailureView::onClickOpenSelectedInScriptWindow() {
  // Open checkpoint in script window
  QList<QTableWidgetItem *> selectedRows = ui->tableWidget->selectedItems();
  if (selectedRows.size() > 0) {
    QString text = selectedRows[0]->text();
    if (text.toStdString() == "") {
      return;
    }
    m_presenter->openSelectedInEditor(text);
  }
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryFailureWindow->OpenSelectedInScriptWindow",
      false);
}

void RecoveryFailureView::onClickStartMantidNormally() {
  // Start save and close this, clear checkpoint that was offered for load
  m_presenter->startMantidNormally();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryFailureWindow->StartMantidNormally", false);
}

void RecoveryFailureView::reject() {
  // Do nothing just absorb request
  m_presenter->startMantidNormally();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "ProjectRecoveryFailureWindow->StartMantidNormally", false);
}