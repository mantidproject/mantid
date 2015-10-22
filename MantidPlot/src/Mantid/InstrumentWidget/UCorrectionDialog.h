#ifndef UCorrectionDIALOG_H
#define REMOVEERRORSDIALOG_H

#include <QDialog>
#include <QPointF>

namespace Ui {
    class UCorrectionDialog;
}

class UCorrectionDialog : public QDialog {
    Q_OBJECT
public:
    UCorrectionDialog(QWidget *parent, QPointF oldValue, bool isManual);
    ~UCorrectionDialog();

    bool applyCorrection() const;
    QPointF getValue() const;

private:
    Ui::UCorrectionDialog *ui;
};

#endif // REMOVEERRORSDIALOG_H
