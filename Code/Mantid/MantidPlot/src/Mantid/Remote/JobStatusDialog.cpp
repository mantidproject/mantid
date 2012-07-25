#include "JobStatusDialog.h"
#include "ui_JobStatusDialog.h"
#include "RemoteJob.h"
#include "RemoteJobManager.h"

#include <QLabel>
#include <QPushButton>

#include <vector>
using namespace std;

JobStatusDialog::JobStatusDialog(const QList <RemoteJob> &jobList, RemoteJobManager *manager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JobStatusDialog),
    m_jobList( jobList),
    m_manager( manager)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setResizeMode( QHeaderView::Stretch);

    /****************************
    // Show a couple of sample jobs....
    ui->tableWidget->setRowCount(2);
    ui->tableWidget->setCellWidget(0, 0, new QLabel("MWS-120"));
    ui->tableWidget->setCellWidget(0, 1, new QLabel("Chadwick"));
    ui->tableWidget->setCellWidget(0, 2, new QLabel("Hello MPI"));
    ui->tableWidget->setCellWidget(0, 3, new QLabel("Complete"));
    ui->tableWidget->setCellWidget(0, 4, new QPushButton("Download",this));

    ui->tableWidget->setCellWidget(1, 0, new QLabel("MWS-121"));
    ui->tableWidget->setCellWidget(1, 1, new QLabel("Chadwick"));
    ui->tableWidget->setCellWidget(1, 2, new QLabel("Hello MPI"));
    ui->tableWidget->setCellWidget(1, 3, new QLabel("Running"));
    //ui->tableWidget->setCellWidget(1, 4, new QPushButton("Download",this));
    *****************************/

    updateDisplay();
}

JobStatusDialog::~JobStatusDialog()
{
    delete ui;
}

void JobStatusDialog::updateDisplay()
{
  ui->tableWidget->setRowCount( 0);

  vector <RemoteJob> jobList;
  string errMsg;

  if (m_manager->jobStatusAll( jobList, errMsg))
  {
    vector <RemoteJob>::iterator it = jobList.begin();
    while (it != jobList.end())
    {
        Mantid::Kernel::time_duration sinceSubmitted = Mantid::Kernel::DateAndTime::getCurrentTime() - (*it).m_submitTime;
        if (sinceSubmitted.hours() < (ui->spinBox->value()*24))
        {
          addRow( *it);
        }
        it++;
    }

  }
}

void JobStatusDialog::addRow( RemoteJob &job)
{
    QTableWidget *table = ui->tableWidget;
    table->setRowCount( table->rowCount() + 1);

    int curRow = table->rowCount() - 1;  // calculate here so we can pass it in to all the calls to setCellWidget()
    table->setCellWidget( curRow, 0, new QLabel( QString::fromStdString(job.m_jobId)));
    table->setCellWidget( curRow, 1, new QLabel( QString::fromStdString(job.m_manager->getDisplayName())));
    table->setCellWidget( curRow, 2, new QLabel( QString::fromStdString(job.m_algName)));

    std::string errMsg;
    RemoteJob::JobStatus status;
    if ( ! job.m_manager->jobStatus( job.m_jobId, status, errMsg))
    {
        // ToDo: should we pop up a dialog box instead?
        status = RemoteJob::JOB_STATUS_UNKNOWN;
    }

    QString statusString;
    switch (status)
    {

    case RemoteJob::JOB_ABORTED:
        statusString = "Aborted";
        break;

    case RemoteJob::JOB_COMPLETE:
        statusString = "Complete";
        break;

    case RemoteJob::JOB_RUNNING:
        statusString = "Running";
        break;

    case RemoteJob::JOB_QUEUED:
        statusString = "Queued";
        break;

    case RemoteJob::JOB_REMOVED:
        statusString = "Removed";
        break;

    case RemoteJob::JOB_STATUS_UNKNOWN:
    default:
        statusString = "Unknown";
        break;
    }


    table->setCellWidget( curRow, 3, new QLabel( statusString));

    if (status == RemoteJob::JOB_COMPLETE)
    {
        table->setCellWidget( curRow, 4, new QPushButton("Download",this));
        // TODO: Hook up the button to a download function!
    }

}

/***************************************************
  this isn't needed anymore (was useful when we were using a
  slider instead of a spinbox
void JobStatusDialog::updateIgnoreVal( int ignoreDays)
{
  m_ignoreDays = ignoreDays;
  if (m_ignoreDays == 1)
    ui->daysLabel->setText( QString("01 ") + QString( QObject::tr("Day")));
  else
    ui->daysLabel->setText( QString("%1 ").arg( m_ignoreDays, 2, 10, QChar('0')) + QString(QObject::tr("Days")));

  updateDisplay();  // Refresh the display
}
*****************************/
