#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqView.h>
#include <pqActiveView.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      CameraManager::CameraManager()
      {
      }

      CameraManager::~CameraManager()
      {
      }

      void CameraManager::getCurrentViewFrustum(double left[4],
                                                double right[4],
                                                double bottom[4],
                                                double top[4],
                                                double far[4],
                                                double near[4])
      {
        pqView * view = pqActiveView::instance().current();

        vtkSMRenderViewProxy* proxy = NULL;

        if (view)
        {
          proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
        }

        if (!proxy)
        {
          // no active view, or active view is not a render view.
          return;
        }

        // Get the aspect ratio of the renderer
        
        vtkRenderer* renderer = proxy->GetRenderer();
        if (!renderer)
        {
          return;
        }

        double aspectDimensions[2];
        renderer->GetAspect(aspectDimensions);

        double aspectRatio = aspectDimensions[0]/aspectDimensions[1];


        // Get the active camera 
        vtkCamera* camera = proxy->GetActiveCamera();

        if (!camera)
        {
          return;
        }

        double planes[24];
        camera->GetFrustumPlanes(aspectRatio, planes);

        for (int k = 0; k < 4; ++k)
        {
          left[k] = planes[k];
          right[k] = planes[k + 4];

          bottom[k] = planes[k + 8];
          top[k] = planes[k + 12];

          far[k] = planes[k + 16];
          near[k] = planes[k + 20];
        }
      }
    }
  }
}