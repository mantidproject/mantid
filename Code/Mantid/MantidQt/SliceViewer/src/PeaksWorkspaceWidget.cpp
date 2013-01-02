#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include <QColorDialog>

namespace MantidQt
{
  namespace SliceViewer
  {
    PeaksWorkspaceWidget::PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, const std::string& coordinateSystem, QWidget *parent)
      : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem)
    {
      ui.setupUi(this);

      ui.tblPeaks->setHidden(true);
      ui.tblPeaks->setFixedHeight(0);
      connect(ui.ckExpand, SIGNAL(clicked(bool)), this, SLOT(onExpandChanged(bool)));
      connect(ui.btnBackgroundColor, SIGNAL(clicked()), this, SLOT(onBackgroundColourClicked()));
      connect(ui.btnPeakColor, SIGNAL(clicked()), this, SLOT(onForegroundColourClicked()));

      populate();
    }

    void PeaksWorkspaceWidget::populate()
    {
      ui.lblWorkspaceName->setText(m_ws->name().c_str());

      const QString unintegratedMsg = "un-integrated";
      const QString integratedMsg = "integrated";

      ui.lblWorkspaceName->setText(m_ws->hasIntegratedPeaks() ? integratedMsg : unintegratedMsg );

      ui.lblWorkspaceCoordinates->setText(m_coordinateSystem.c_str());
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

  } // namespace
}
