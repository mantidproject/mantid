#include "ProjectRecoveryView.h"
#include "ui_ProjectRecoveryWidget.h"

ProjectRecoveryView::ProjectRecoveryView(QWidget *parent,
                                         ProjectRecoveryPresenter *presenter)
    : QWidget(parent), ui(new Ui::ProjectRecoveryWidget),
      m_presenter(presenter) {
  ui->setupUi(this);
  ui->tableWidget->horizontalHeader()->setResizeMode(
      QHeaderView::Stretch);
  ui->tableWidget->verticalHeader()->setResizeMode(
      QHeaderView::Stretch);
  ui->tableWidget->verticalHeader()->setVisible(false);
  // Set the table information
  QStringList row = presenter->getRow(0);
  ui->tableWidget->setItem(0, 0, new QTableWidgetItem(row[0]));
  ui->tableWidget->setItem(0, 1, new QTableWidgetItem(row[1]));
}

ProjectRecoveryView::~ProjectRecoveryView() { delete ui; }

void ProjectRecoveryView::onClickLastCheckpoint() {
  // Recover last checkpoint
  m_presenter->recoverLast();
}

void ProjectRecoveryView::onClickOpenLastInScriptWindow() {
  // Open checkpoint in script window
  m_presenter->openLastInEditor();
}

void ProjectRecoveryView::onClickStartMantidNormally() {
  // Start save and close this, clear checkpoint that was offered for load
  m_presenter->startMantidNormally();
}
