#ifndef LINEVIEWER_H
#define LINEVIEWER_H

#include "DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/VMD.h"
#include "ui_LineViewer.h"
#include <QtGui/QWidget>
#include <qwt_plot_curve.h>
#include <qwt_plot.h>
#include <qwt_painter.h>
#include "MantidAPI/CoordTransform.h"
#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidQtSliceViewer/LinePlotOptions.h"
#include "MantidQtAPI/AlgorithmRunner.h"

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
    void setThickness(Mantid::Kernel::VMD width);
    void setPlanarWidth(double width);
    void setNumBins(int numBins);
    void setFixedBinWidthMode(bool fixedWidth, double binWidth);
    void setPlotAxis(int choice);

    void showPreview();
    void showFull();

    double getPlanarWidth() const;
    Mantid::Kernel::VMD getWidth() const;
    double getFixedBinWidth() const;
    bool getFixedBinWidthMode() const;
    int getNumBins() const;
    double getBinWidth() const;
    int getPlotAxis() const;

    // For python
    void setStartXY(double x, double y);
    void setEndXY(double x, double y);
    void setThickness(double width);
    void setThickness(int dim, double width);
    void setThickness(const QString & dim, double width);
    QPointF getStartXY() const;
    QPointF getEndXY() const;
    int getXAxisDimensionIndex() const;

private:
    void createDimensionWidgets();
    void updateFreeDimensions();
    void updateStartEnd();
    void updateBinWidth();
    void readTextboxes();
    bool isLogScaledY() const;

public slots:
    void startEndTextEdited();
    void thicknessTextEdited();
    void startLinkedToEndText();
    void apply();
    void numBinsChanged();
    void adaptiveBinsChanged();
    void setFreeDimensions(size_t dimX, size_t dimY);
    void on_radNumBins_toggled();
    void textBinWidth_changed();
    void refreshPlot();
    void lineIntegrationComplete(bool error);
    void onToggleLogYAxis();

signals:
    /// Signal emitted when the planar width changes
    void changedPlanarWidth(double);
    /// Signal emitted when the start or end position has changed
    void changedStartOrEnd(Mantid::Kernel::VMD, Mantid::Kernel::VMD);
    /// Signal emitted when changing fixed bin width mode
    void changedFixedBinWidth(bool, double);

private:
    Mantid::API::IAlgorithm_sptr applyMDWorkspace(Mantid::API::IMDWorkspace_sptr ws);
    Mantid::API::IAlgorithm_sptr applyMatrixWorkspace(Mantid::API::MatrixWorkspace_sptr ws);
    void setupScaleEngine(MantidQwtWorkspaceData& curveData);

    // -------------------------- Widgets ----------------------------

    /// Auto-generated UI controls.
    Ui::LineViewerClass ui;

    /// Layout containing the plot
    QVBoxLayout * m_plotLayout;

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
    /// Vector of text boxes with the thicknesses
    QVector<QLineEdit *> m_thicknessText;

    /// Widget to choose X plot axis and normalization
    LinePlotOptions * m_lineOptions;

    /// Object for running algorithms in the background
    MantidQt::API::AlgorithmRunner * m_algoRunner;

    // -------------------------- Data Members ----------------------------

    /// Workspace being sliced
    Mantid::API::IMDWorkspace_sptr m_ws;

    /// Workspace of the slice
    Mantid::API::IMDWorkspace_sptr m_sliceWS;

    /// Name of the workspace that was integrated (asynchronously).
    std::string m_integratedWSName;

    /// Start point of the line
    Mantid::Kernel::VMD m_start;
    /// End point of the line
    Mantid::Kernel::VMD m_end;
    /// Width in each dimension (some will be ignored)
    Mantid::Kernel::VMD m_thickness;
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

    /// Index of the first selected X dimension in the 2D slice
    int m_initFreeDimX;
    /// Index of the first selected Y dimension in the 2D slice
    int m_initFreeDimY;

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
