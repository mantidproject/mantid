#ifndef JOBSTATUSDIALOG_H
#define JOBSTATUSDIALOG_H

#include "RemoteJobManager.h"

#include <QDialog>
#include <QSignalMapper>

class RemoteJob;
class MantidUI;

namespace Ui {
    class JobStatusDialog;
}

class JobStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JobStatusDialog(const QList <RemoteJob> &jobList, RemoteJobManager *manager, MantidUI *mantidui, QWidget *parent = 0);
    ~JobStatusDialog();

    bool readyToDisplay() const { return m_displayReady; }

protected:
    void addRow( RemoteJob &job);

protected slots:
    void updateDisplay();
    void downloadFile( QString jobId);
    /******************
      not needed any more...
    void updateIgnoreVal( int ignoreDays);
    ****************/
private:
    MantidUI *m_mantidUI;
    Ui::JobStatusDialog *ui;

    QList <RemoteJob> m_jobList;  // list of jobs that have been submitted.  Not sure we'll
                                  // need this going forward...
    QSignalMapper *m_buttonMap;    // Maps all the download buttons to their job ID's
    RemoteJobManager *m_manager;
    int m_ignoreDays; // Jobs older than this number of days will not be displayed
    bool m_displayReady;
};

#endif // JOBSTATUSDIALOG_H
