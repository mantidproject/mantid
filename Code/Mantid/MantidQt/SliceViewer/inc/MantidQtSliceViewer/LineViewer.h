#ifndef LINEVIEWER_H
#define LINEVIEWER_H

#include <QtGui/QWidget>
#include "ui_LineViewer.h"
#include "MantidAPI/IMDWorkspace.h"
#include <qwt_plot.h>
#include "MantidKernel/VMD.h"

namespace MantidQt
{
namespace SliceViewer
{

class LineViewer : public QWidget
{
    Q_OBJECT

public:
    LineViewer(QWidget *parent = 0);
    ~LineViewer();

    void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
    void setStart(Mantid::Kernel::VMD start);
    void setEnd(Mantid::Kernel::VMD end);
    void setNumBins(size_t numBins);

    void showPreview();

private:
    void createDimensionWidgets();

private:
    // -------------------------- Widgets ----------------------------

    /// Auto-generated UI controls.
    Ui::LineViewerClass ui;

    /// Layout containing the plot
    QHBoxLayout * m_plotLayout;

    /// Main plot object
    QwtPlot * m_plot;

    /// Vector of labels with the dimension names
    QVector<QLabel *> m_dimensionLabel;
    /// Vector of text boxes with the start point
    QVector<QLineEdit *> m_startText;
    /// Vector of text boxes with the end point
    QVector<QLineEdit *> m_endText;
    /// Vector of text boxes with the widths
    QVector<QLineEdit *> m_widthText;


    // -------------------------- Data Members ----------------------------

    /// Workspace being shown
    Mantid::API::IMDWorkspace_sptr m_ws;

    /// Start point of the line
    Mantid::Kernel::VMD m_start;
    /// End point of the line
    Mantid::Kernel::VMD m_end;

    /// Number of bins (for regular spacing)
    size_t m_numBins;

};

} //namespace
}
#endif // LINEVIEWER_H
