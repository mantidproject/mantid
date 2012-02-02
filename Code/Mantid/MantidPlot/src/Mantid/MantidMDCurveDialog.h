#ifndef MANTIDMDCURVEDIALOG_H
#define MANTIDMDCURVEDIALOG_H

#include <QtGui/QWidget>
#include "ui_MantidMDCurveDialog.h"
#include "MantidQtSliceViewer/LinePlotOptions.h"

/** Dialog asking the user for some options on how to plot a MDWorkspace
 * before plotting
 */
class MantidMDCurveDialog : public QDialog
{
    Q_OBJECT

public:
    MantidMDCurveDialog(QWidget *parent = 0, QString wsName=QString());
    ~MantidMDCurveDialog();

    LinePlotOptions * getLineOptionsWidget()
    {return m_lineOptions; }

    bool showErrorBars();

public slots:
    void on_btnOK_clicked();
    void on_btnCancel_clicked();

private:
    Ui::MantidMDCurveDialogClass ui;

    /// Name of the workspace to plot
    QString m_wsName;

    /// Widget with MD plot options
    LinePlotOptions * m_lineOptions;
};

#endif // MANTIDMDCURVEDIALOG_H
