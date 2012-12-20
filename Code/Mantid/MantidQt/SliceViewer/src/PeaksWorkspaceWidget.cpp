#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"

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
      connect(ui.ckExpand, SIGNAL(clicked(bool)), this, SLOT(expandChanged(bool)));

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

    void PeaksWorkspaceWidget::expandChanged(bool open)
    {
      ui.tblPeaks->setHidden(!open);
      ui.tblPeaks->setFixedHeight(QWIDGETSIZE_MAX);
    }

  } // namespace
}
