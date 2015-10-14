#include "MantidVatesSimpleGuiViewWidgets/SaveScreenshotReaction.h"

#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <pqImageUtil.h>
#include <pqPVApplicationCore.h>
#include <pqRenderViewBase.h>
#include <pqSaveSnapshotDialog.h>
#include <pqSettings.h>
#include <pqTabbedMultiViewWidget.h>
#include <pqView.h>
#include "MantidVatesAPI/vtkImageData_Silent.h"
#include <vtkPVXMLElement.h>
#include <vtkSmartPointer.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkPVConfig.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QDebug>
#include <QFileInfo>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

//-----------------------------------------------------------------------------
SaveScreenshotReaction::SaveScreenshotReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
                   this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)),
                   this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void SaveScreenshotReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  bool is_enabled = (activeObjects->activeView() && activeObjects->activeServer());
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
void SaveScreenshotReaction::saveScreenshot()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannnot save image. No active view.";
    return;
  }

  pqSaveSnapshotDialog ssDialog(pqCoreUtilities::mainWidget());
  ssDialog.setViewSize(view->getSize());

  if (ssDialog.exec() != QDialog::Accepted)
  {
    return;
  }

  QString lastUsedExt;
  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("extensions/ScreenshotExtension"))
  {
    lastUsedExt =
        settings->value("extensions/ScreenshotExtension").toString();
  }

  QString filters;
  filters += "PNG image (*.png)";
  filters += ";;BMP image (*.bmp)";
  filters += ";;TIFF image (*.tif)";
  filters += ";;PPM image (*.ppm)";
  filters += ";;JPG image (*.jpg)";
  filters += ";;PDF file (*.pdf)";
  pqFileDialog file_dialog(NULL,
                           pqCoreUtilities::mainWidget(),
                           tr("Save Screenshot:"), QString(), filters);
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName("FileSaveScreenshotDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo = QFileInfo( file );
  lastUsedExt = QString("*.") + fileInfo.suffix();
  settings->setValue("extensions/ScreenshotExtension", lastUsedExt);

  QSize size = ssDialog.viewSize();
  QString palette = ssDialog.palette();

  vtkSMSessionProxyManager *pxm =
      pqActiveObjects::instance().activeServer()->proxyManager();
  vtkSMProxy *colorPalette = pxm->GetProxy("global_properties", "ColorPalette");
  vtkSmartPointer<vtkSMProxy> clone;
  if (colorPalette && !palette.isEmpty())
  {
    // save current property values
    clone.TakeReference(pxm->NewProxy(colorPalette->GetXMLGroup(),
                                      colorPalette->GetXMLName()));
    clone->Copy(colorPalette);

    vtkSMProxy *chosenPalette = pxm->NewProxy("palettes",
                                              palette.toLatin1().data());
    colorPalette->Copy(chosenPalette);
    chosenPalette->Delete();
  }

  int stereo = ssDialog.getStereoMode();
  if (stereo)
  {
    pqRenderViewBase::setStereo(stereo);
  }

  SaveScreenshotReaction::saveScreenshot(file, size, ssDialog.quality());

  // restore color palette.
  if (clone)
  {
    colorPalette->Copy(clone);
  }

  // restore stereo
  if (stereo)
  {
    pqRenderViewBase::setStereo(0);
  }

  // check if need to render to clear the changes we did
  // while saving the screenshot.
  if (clone || stereo)
  {
    pqApplicationCore::instance()->render();
  }
}

//-----------------------------------------------------------------------------
void SaveScreenshotReaction::saveScreenshot(const QString& filename,
                                            const QSize& size, int quality)
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSmartPointer<vtkImageData> img;
  if (view)
  {
    img.TakeReference(view->captureImage(size));
  }

  if (img.GetPointer() == NULL)
  {
    qCritical() << "Save Image failed.";
  }
  else
  {
    pqImageUtil::saveImage(img, filename, quality);
  }
}

}
}
}
