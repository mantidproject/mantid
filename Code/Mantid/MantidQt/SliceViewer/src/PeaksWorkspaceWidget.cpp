#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    PeaksWorkspaceWidget::PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, QWidget *parent)
      : QWidget(parent), m_ws(ws)
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

      // Viewing coordinate system.... ? SliceViewer::peakCoordinateSystem() -> PeakTransformSelector -> PeakTransformFactory::productName(instancemethod) -> PeakTransform::name(static) 
      // Default Background colour .... ?  -- PeakPallette ?
      // Default Foreground colour .... ?  -- PeakPallette ?
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
