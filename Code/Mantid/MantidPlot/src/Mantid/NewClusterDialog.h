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

    const QString getDisplayName() const;
    const QUrl getServiceBaseURL() const;
    const QUrl getConfigFileURL() const;
    const QString getUserName() const;
private slots:    
    bool validateInput() const;  // Will enable the OK button if the input is valid
private:
    Ui::NewClusterDialog *ui;
};

#endif // NEWCLUSTERDIALOG_H
