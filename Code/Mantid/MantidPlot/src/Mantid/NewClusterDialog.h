#ifndef NEWCLUSTERDIALOG_H
#define NEWCLUSTERDIALOG_H

#include <QDialog>
#include <QString>
#include <QUrl>

namespace Ui {
    class NewClusterDialog;
}

class NewClusterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewClusterDialog(QWidget *parent = 0);
    ~NewClusterDialog();

    const QString getDisplayName();
    const QUrl getServiceBaseURL();
    const QUrl getConfigFileURL();
private slots:    
    bool validateInput();  // Will enable the OK button if the input is valid
private:
    Ui::NewClusterDialog *ui;
};

#endif // NEWCLUSTERDIALOG_H
