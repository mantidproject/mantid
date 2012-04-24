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

private:
    Ui::JobStatusDialog *ui;
};

#endif // JOBSTATUSDIALOG_H
