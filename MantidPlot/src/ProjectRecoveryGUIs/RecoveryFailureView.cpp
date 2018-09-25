#include "RecoveryFailureView.h"
#include "ui_RecoveryFailure.h"

RecoveryFailureView::RecoveryFailureView(QWidget *parent, ProjectRecoveryPresenter *presenter) :
    QWidget(parent),
    ui(new Ui::RecoveryFailure)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    // Set the table information
    addDataToTable(ui);
}

RecoveryFailureView::~RecoveryFailureView()
{
    delete ui;
}

void RecoveryFailureView::addDataToTable(Ui::RecoveryFailure* ui){
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

void RecoveryFailureView::onClickLastCheckpoint(){
    // Recover last checkpoint
    m_presenter->recoverLast();
}

void RecoveryFailureView::onClickSelectedCheckpoint(){
    // Recover Selected
    // m_presenter->recoverSelectedCheckpoint();
}

void RecoveryFailureView::onClickOpenSelectedInScriptWindow(){
    // Open checkpoint in script window
    // m_presenter->openSelectedInEditor();
}

void RecoveryFailureView::onClickStartMantidNormally(){
    // Start save and close this, clear checkpoint that was offered for load
    m_presenter->startMantidNormally();
}

