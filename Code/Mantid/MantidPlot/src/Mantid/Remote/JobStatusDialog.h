#ifndef JOBSTATUSDIALOG_H
#define JOBSTATUSDIALOG_H

#include <QDialog>

class RemoteJob;

namespace Ui {
    class JobStatusDialog;
}

class JobStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JobStatusDialog(QWidget *parent = 0);
    ~JobStatusDialog();
    void addRow( RemoteJob &job);

public slots:
    void updateIgnoreVal( int ignoreDays);

private:
    Ui::JobStatusDialog *ui;

    int m_ignoreDays; // Jobs older than this number of days will not be displayed
};

#endif // JOBSTATUSDIALOG_H
