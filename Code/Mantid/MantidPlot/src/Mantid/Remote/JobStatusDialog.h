#ifndef JOBSTATUSDIALOG_H
#define JOBSTATUSDIALOG_H

#include "RemoteJobManager.h"

#include <QDialog>

class RemoteJob;

namespace Ui {
    class JobStatusDialog;
}

class JobStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JobStatusDialog(const QList <RemoteJob> &jobList, RemoteJobManager *manager, QWidget *parent = 0);
    ~JobStatusDialog();

protected:
    void addRow( RemoteJob &job);

protected slots:
    void updateDisplay();
    /******************
      not needed any more...
    void updateIgnoreVal( int ignoreDays);
    ****************/
private:
    Ui::JobStatusDialog *ui;

    QList <RemoteJob> m_jobList;  // list of jobs that have been submitted.  Not sure we'll
                                  // need this going forward...
    RemoteJobManager *m_manager;
    int m_ignoreDays; // Jobs older than this number of days will not be displayed
};

#endif // JOBSTATUSDIALOG_H
