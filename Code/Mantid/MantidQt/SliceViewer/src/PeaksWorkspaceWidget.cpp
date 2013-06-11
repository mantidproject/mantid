#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtSliceViewer/QPeaksTableModel.h"
#include <QColorDialog>
#include <QPlastiqueStyle>

namespace MantidQt
{
  namespace SliceViewer
  {

    /**
    Constructor

    @param ws : Peaks Workspace (MODEL)
    @param coordinateSystem : Name of coordinate system used
    @param defaultForegroundColour : Default peak foreground colour
    @param defaultBackgroundColour : Default peak background colour
    @param parent : parent widget
    */
    PeaksWorkspaceWidget::PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, const std::string& coordinateSystem, const QColor& defaultForegroundColour, const QColor& defaultBackgroundColour, QWidget *parent)
      : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem), m_foregroundColour(defaultForegroundColour), m_backgroundColour(defaultBackgroundColour)
    {

      ui.setupUi(this);

      // Connect internal signals-slots.
      connect(ui.ckShowBackground, SIGNAL(clicked(bool)), this, SLOT(onShowBackgroundChanged(bool)));
      connect(ui.btnBackgroundColor, SIGNAL(clicked()), this, SLOT(onBackgroundColourClicked()));
      connect(ui.btnPeakColor, SIGNAL(clicked()), this, SLOT(onForegroundColourClicked()));
      connect(ui.btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveWorkspaceClicked()));
      connect(ui.btnHide, SIGNAL(toggled(bool)), this, SLOT(onToggleHideInPlot(bool)));
      connect(ui.tblPeaks, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onTableClicked(const QModelIndex&)));

      // Override the styles for the colour buttons, because with some inherited styles, the button background colour will be hidden.
      ui.btnBackgroundColor->setStyle(new QPlastiqueStyle);
      ui.btnPeakColor->setStyle(new QPlastiqueStyle);

      // Hide controls that don't apply when peaks are integrated.
      const bool integratedPeaks = m_ws->hasIntegratedPeaks();
      ui.btnBackgroundColor->setVisible(integratedPeaks);
      ui.ckShowBackground->setVisible(integratedPeaks);
      ui.lblShowBackgroundColour->setVisible(integratedPeaks);

      // Populate controls with data.
      populate();
    }

    /**
    Populate controls with data ready for rendering.
    */
    void PeaksWorkspaceWidget::populate()
    {
      const QString nameText = QString(m_ws->name().c_str());
      ui.lblWorkspaceName->setText(nameText);
      ui.lblWorkspaceName->setToolTip(nameText);

      const QString integratedText = m_ws->hasIntegratedPeaks() ? "Yes" : "No";

      ui.lblWorkspaceState->setText(integratedText);
      ui.lblWorkspaceState->setToolTip(integratedText);

      const QString coordinateText = QString(m_coordinateSystem.c_str());
      ui.lblWorkspaceCoordinates->setText(coordinateText);
      ui.lblWorkspaceCoordinates->setToolTip(coordinateText);

      ui.btnBackgroundColor->setBackgroundColor(m_backgroundColour);
      ui.btnPeakColor->setBackgroundColor(m_foregroundColour);

      auto model = new QPeaksTableModel(this->m_ws);
      connect(model, SIGNAL(peaksSorted(const std::string&, const bool)), this, SLOT(onPeaksSorted(const std::string&, const bool)));
      ui.tblPeaks->setModel(model);
      ui.tblPeaks->verticalHeader()->setResizeMode(QHeaderView::Interactive);
      ui.tblPeaks->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
      m_originalTableWidth = ui.tblPeaks->horizontalHeader()->length();

      // set the starting width of each column
      int char_width = (ui.tblPeaks->fontMetrics().maxWidth()+ui.tblPeaks->fontMetrics().averageCharWidth());
      char_width = static_cast<int>(.4*static_cast<double>(char_width)); // randomly determined "correct"
      for (int i = 0; i < m_originalTableWidth; ++i)
      {
        ui.tblPeaks->horizontalHeader()->resizeSection(i, model->numCharacters(i) * char_width);
      }

    }

    /// Destructor
    PeaksWorkspaceWidget::~PeaksWorkspaceWidget()
    {
    }

    /**
    Handler for changing the foreground colour of an integrated peak.
    */
    void PeaksWorkspaceWidget::onForegroundColourClicked()
    {
      QColorDialog colourDlg;
      QColor selectedColour = colourDlg.getColor();

      ui.btnPeakColor->setBackgroundColor(selectedColour);
      emit peakColourChanged(this->m_ws, selectedColour);
    }

    /**
    Handler for changing the background colour of an integrated peak.
    */
    void PeaksWorkspaceWidget::onBackgroundColourClicked()
    {
      QColorDialog colourDlg;
      QColor selectedColour = colourDlg.getColor();
      ui.btnBackgroundColor->setBackgroundColor(selectedColour);
      emit backgroundColourChanged(this->m_ws, selectedColour);
    }

    /**
    Handler or showing/hiding the background radius of integrated peaks.
    @param show : TRUE if the background radius is to be shown.
    */
    void PeaksWorkspaceWidget::onShowBackgroundChanged(bool show)
    {
      emit backgroundRadiusShown(this->m_ws, show);
    }

    /**
    Handler for removing a workspace from the plotting tools.
    */
    void PeaksWorkspaceWidget::onRemoveWorkspaceClicked()
    {
      emit removeWorkspace(this->m_ws);
      this->hide();
    }

    /**
    Handler to hide/show the widget on request.
    @param hidden: flag indicating what to do.
    */
    void PeaksWorkspaceWidget::onToggleHideInPlot(bool hidden)
    {
      emit hideInPlot(this->m_ws, hidden);
    }

    /**
    Event handler for handling the user seleting a row in the table.
    @param index : Index selected.
    */
    void PeaksWorkspaceWidget::onTableClicked(const QModelIndex& index)
    {
      if(index.isValid())
      {
        emit zoomToPeak(this->m_ws, index.row());
      }
    }

    /**
     * Handler for sorting of the peaks workspace.
     * @param columnToSortBy
     * @param sortAscending
     */
    void PeaksWorkspaceWidget::onPeaksSorted(const std::string& columnToSortBy, const bool sortAscending)
    {
      emit peaksSorted(columnToSortBy, sortAscending, this->m_ws);
    }

  } // namespace
}
