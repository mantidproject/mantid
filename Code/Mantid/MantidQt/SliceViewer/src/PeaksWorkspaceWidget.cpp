#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include <QColorDialog>

namespace MantidQt
{
  namespace SliceViewer
  {
    PeaksWorkspaceWidget::PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, const std::string& coordinateSystem, const QColor& defaultForegroundColour, const QColor& defaultBackgroundColour, QWidget *parent)
      : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem), m_foregroundColour(defaultForegroundColour), m_backgroundColour(defaultBackgroundColour)
    {
      ui.setupUi(this);

      ui.tblPeaks->setHidden(true);
      ui.tblPeaks->setFixedHeight(0);
      connect(ui.ckExpand, SIGNAL(clicked(bool)), this, SLOT(onExpandChanged(bool)));
      connect(ui.ckShowBackground, SIGNAL(clicked(bool)), this, SLOT(onShowBackgroundChanged(bool)));
      connect(ui.btnBackgroundColor, SIGNAL(clicked()), this, SLOT(onBackgroundColourClicked()));
      connect(ui.btnPeakColor, SIGNAL(clicked()), this, SLOT(onForegroundColourClicked()));
   
      populate();
    }

    void PeaksWorkspaceWidget::populate()
    {
      const QString nameText = "Workspace name: " +  QString(m_ws->name().c_str());
      ui.lblWorkspaceName->setText(nameText);

      const QString integratedMsg = m_ws->hasIntegratedPeaks() ? "Yes" : "No";
      const QString integratedText = "Peaks Integrated: " + integratedMsg;

      ui.lblWorkspaceName->setText(integratedText);

      const QString coordinateText = "Coordinate system: " + QString(m_coordinateSystem.c_str());
      ui.lblWorkspaceCoordinates->setText(coordinateText);

      ui.btnBackgroundColor->setBackgroundColor(m_backgroundColour);
      ui.btnPeakColor->setBackgroundColor(m_foregroundColour);
    }

    PeaksWorkspaceWidget::~PeaksWorkspaceWidget()
    {
    }

    void PeaksWorkspaceWidget::onExpandChanged(bool open)
    {
      ui.tblPeaks->setHidden(!open);
      ui.tblPeaks->setFixedHeight(QWIDGETSIZE_MAX);
    }

    void PeaksWorkspaceWidget::onForegroundColourClicked()
    {
      QColorDialog colourDlg;
      QColor selectedColour = colourDlg.getColor();
      ui.btnPeakColor->setBackgroundColor(selectedColour);
      emit peakColourChanged(this->m_ws, selectedColour);
    }

    void PeaksWorkspaceWidget::onBackgroundColourClicked()
    {
      QColorDialog colourDlg;
      QColor selectedColour = colourDlg.getColor();
      ui.btnBackgroundColor->setBackgroundColor(selectedColour);
      emit backgroundColourChanged(this->m_ws, selectedColour);
    }

    void PeaksWorkspaceWidget::onShowBackgroundChanged(bool show)
    {
      emit backgroundRadiusShown(this->m_ws, show);
    }

  } // namespace
}
