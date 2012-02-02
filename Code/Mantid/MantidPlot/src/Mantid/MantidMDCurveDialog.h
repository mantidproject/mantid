#ifndef MANTIDMDCURVEDIALOG_H
#define MANTIDMDCURVEDIALOG_H

#include <QtGui/QWidget>
#include "ui_MantidMDCurveDialog.h"
#include "MantidQtSliceViewer/LinePlotOptions.h"

/** Dialog asking the user for some options on how to plot a MDWorkspace
 * before plotting
 */
class MantidMDCurveDialog : public QWidget
{
    Q_OBJECT

public:
    MantidMDCurveDialog(QWidget *parent = 0, QString wsName=QString());
    ~MantidMDCurveDialog();

private:
    Ui::MantidMDCurveDialogClass ui;

    /// Name of the workspace to plot
    QString m_wsName;

    /// Widget with MD plot options
    LinePlotOptions * m_lineOptions;
};

#endif // MANTIDMDCURVEDIALOG_H
