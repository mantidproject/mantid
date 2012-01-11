#ifndef LINEVIEWER_H
#define LINEVIEWER_H

#include "DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/VMD.h"
#include "ui_LineViewer.h"
#include <QtGui/QWidget>
#include <qwt_plot_curve.h>
#include <qwt_plot.h>

namespace MantidQt
{
namespace SliceViewer
{

class EXPORT_OPT_MANTIDQT_SLICEVIEWER LineViewer : public QWidget
{
    Q_OBJECT

public:
    LineViewer(QWidget *parent = 0);
    ~LineViewer();

    void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
    void setFreeDimensions(bool all, int dimX, int dimY);
    void setStart(Mantid::Kernel::VMD start);
    void setEnd(Mantid::Kernel::VMD end);
    void setWidth(Mantid::Kernel::VMD width);
    void setPlanarWidth(double width);
    void setNumBins(int numBins);
    void setFixedBinWidthMode(bool fixedWidth, double binWidth);

    void showPreview();
    void showFull();

    double getPlanarWidth() const;
    Mantid::Kernel::VMD getWidth() const;
    double getFixedBinWidth() const;
    bool getFixedBinWidthMode() const;
    int getNumBins() const;
    double getBinWidth() const;

    // For python
    void setStartXY(double x, double y);
    void setEndXY(double x, double y);
    void setWidth(double width);
    void setWidth(int dim, double width);
    void setWidth(const QString & dim, double width);

private:
    void createDimensionWidgets();
    void updateFreeDimensions();
    void updateStartEnd();
    void updateBinWidth();
    void readTextboxes();
    void calculateCurve(Mantid::API::IMDWorkspace_sptr ws, Mantid::Kernel::VMD start, Mantid::Kernel::VMD end,
        size_t minNumPoints, QwtPlotCurve * curve);

public slots:
    void startEndTextEdited();
    void widthTextEdited();
    void startLinkedToEndText();
    void apply();
    void numBinsChanged();
    void adaptiveBinsChanged();
    void setFreeDimensions(size_t dimX, size_t dimY);
    void on_radNumBins_toggled();
    void textBinWidth_changed();

signals:
    /// Signal emitted when the planar width changes
    void changedPlanarWidth(double);
    /// Signal emitted when the start or end position has changed
    void changedStartOrEnd(Mantid::Kernel::VMD, Mantid::Kernel::VMD);
    /// Signal emitted when changing fixed bin width mode
    void changedFixedBinWidth(bool, double);


private:
    // -------------------------- Widgets ----------------------------

    /// Auto-generated UI controls.
    Ui::LineViewerClass ui;

    /// Layout containing the plot
    QHBoxLayout * m_plotLayout;

    /// Main plot object
    QwtPlot * m_plot;

    /// Curve of the preview
    QwtPlotCurve * m_previewCurve;

    /// Curve of the full integrated
    QwtPlotCurve * m_fullCurve;

    /// Vector of labels with the dimension names
    QVector<QLabel *> m_dimensionLabel;
    /// Vector of text boxes with the start point
    QVector<QLineEdit *> m_startText;
    /// Vector of text boxes with the end point
    QVector<QLineEdit *> m_endText;
    /// Vector of text boxes with the widths
    QVector<QLineEdit *> m_widthText;


    // -------------------------- Data Members ----------------------------

    /// Workspace being sliced
    Mantid::API::IMDWorkspace_sptr m_ws;

    /// Workspace of the slice
    Mantid::API::IMDWorkspace_sptr m_sliceWS;

    /// Start point of the line
    Mantid::Kernel::VMD m_start;
    /// End point of the line
    Mantid::Kernel::VMD m_end;
    /// Width in each dimension (some will be ignored)
    Mantid::Kernel::VMD m_width;
    /// Width in the in-plane, perpendicular-to-line direction
    double m_planeWidth;


    /// Number of bins (for regular spacing)
    size_t m_numBins;

    /// Flag that is true when all dimensions are allowed to change
    bool m_allDimsFree;
    /// Index of the X dimension in the 2D slice
    int m_freeDimX;
    /// Index of the Y dimension in the 2D slice
    int m_freeDimY;

    /// When True, then the bin width is fixed and the number of bins changes
    bool m_fixedBinWidthMode;

    /// Desired bin width in fixedBinWidthMode
    double m_fixedBinWidth;

    /// ACTUAL bin width, whether in fixed or not-fixed bin width mode
    double m_binWidth;

};

} //namespace
}
#endif // LINEVIEWER_H
