#include "JobStatusDialog.h"
#include "ui_JobStatusDialog.h"
#include "MantidRemote/RemoteJob.h"
#include "MantidRemote/RemoteJobManager.h"
#include "Mantid/MantidUI.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QLabel>
#include <QPushButton>
#include <QTemporaryFile>
#include <QDataStream>
#include <QMessageBox>

#include <fstream>
#include <vector>
using namespace std;

JobStatusDialog::JobStatusDialog( RemoteJobManager *manager, MantidUI *mantidui, QWidget *parent) :
    QDialog(parent),
    m_mantidUI(mantidui),
    ui(new Ui::JobStatusDialog),
    m_manager( manager),
    m_displayReady( false)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setResizeMode( QHeaderView::Stretch);

    m_buttonMap = new QSignalMapper(this);
    connect( m_buttonMap, SIGNAL(mapped(QString)), this, SLOT(downloadFile(QString)));

    updateDisplay();
}

JobStatusDialog::~JobStatusDialog()
{
    delete ui;
}

void JobStatusDialog::updateDisplay()
{
  ui->tableWidget->setRowCount( 0);
  // Note: this will also remove all the mappings from m_buttonMap (Mappings are
  // removed automatically when the mapped objects are destroyed.)

  vector <RemoteJob> jobList;
  string errMsg;

  if (m_manager->jobStatusAll( jobList, errMsg))
  {
    sort( jobList.begin(), jobList.end());  // Sorts the list by job id
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

    m_displayReady = true;
  }
  else
  {
      // D'oh!  There was some kind of error querying the jobs.  The
      // errMsg string should have some kind of explanation.  Display it
      // in (yet another) dialog box.
      QMessageBox msgBox;
      msgBox.setText(tr("Job query failed."));
      msgBox.setInformativeText( QString::fromStdString( errMsg));
      msgBox.exec();
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

    case RemoteJob::JOB_DEFERRED:
        statusString = "Deferred";
        break;

    case RemoteJob::JOB_STATUS_UNKNOWN:
    default:
        statusString = "Unknown";
        break;
    }


    table->setCellWidget( curRow, 3, new QLabel( statusString));

    if (status == RemoteJob::JOB_COMPLETE)
    {
#if 0
      ifdef'd out because jobOutputReady doesn't exist anymore
        // check to see if there's an output file and if it's readable.
        if (m_manager->jobOutputReady( job.m_jobId))
        {
            QPushButton *pb = new QPushButton("Download",this);
            table->setCellWidget( curRow, 4, pb);
            m_buttonMap->setMapping(pb, QString::fromStdString( job.m_jobId));
            connect( pb, SIGNAL(clicked()), m_buttonMap, SLOT(map()));
        }
#endif
    }

}


// Retrieve the output file and load it into a new workspace
void JobStatusDialog::downloadFile( QString jobId)
{

#if 0
  ifdef out because the file download stuff if being re-worked (and also
  because the whole dialog box needs to be re-written)

  // Until we get around to re-writing the whole loader subsystem to accept a
  // stream, we'll have to save the file to a temp directory and then pass
  // the filename into the loader class.

  // This is a bit of a kludge:  I want to use QTemporaryFile, but I don't want
  // to use QT specific stuff in m_manager.  So, I'm going to close the file
  // that the QTemporaryFile object creates, then re-open it using a standard
  // C++ stream.

  // Use a temporary file that will end in .nxs (so the file loader algorithm
  // will recognize it)
  //QTemporaryFile outfile( QDir::tempPath() + "/workspace.XXXXXX.nxs");
  //outfile.open();  // The file isn't actually created until open() is called
  //outfile.close();
  //ofstream outstream;
  //outstream.open( outfile.fileName());

  // HACK: Can't use QTemporaryFile because the actual workspace load occurs in another thread
  // which means the QTemporaryFile object goes out of scope and deletes the file before
  // the load algorithm has a chance to run.
  // Using a hard-code output stream for now....
  ofstream outstream( (QDir::tempPath() + "/downloaded_workspace.nxs").toStdString());

  bool fileDownloaded = m_manager->getJobOutput( jobId.toStdString(), outstream);
  outstream.close();

  // This bit was mostly copied from ApplicationWindow::loadDataFile()
  if (fileDownloaded)
  {
    // Run Load algorithm on file
    QMap<QString,QString> params;
    // HACK! See above about the hard-coded file name...
    //params["Filename"] = outfile.fileName();
    params["Filename"] = QDir::tempPath() + "/downloaded_workspace.nxs";
    m_mantidUI->executeAlgorithmDlg("Load",params);
  }
  else
  {
    // error downloading the file
    QMessageBox ( QMessageBox::Warning, "Download Error", QString("Failed to download output for job ID ") + jobId, QMessageBox::Ok, this).exec();
  }


  // Note: QTemporaryFile automatically deletes the file when the object
  // goes out of scope.  Nice!
#endif

  return;
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
