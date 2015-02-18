#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <stdexcept>
// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqView.h>
#include <pqActiveView.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

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
      /**
       * Get the plane equation for the view frustum.
       * @param left The left plane.
       * @param right The right plane.
       * @param bottom The bottom plane.
       * @param top The top plane.
       * @param far The far plane.
       * @param near The near plane.
       */
      Mantid::VATES::ViewFrustum CameraManager::getCurrentViewFrustum()
      {
        double left[4];
        double right[4];
        double bottom[4];
        double top[4];
        double far[4];
        double near[4];

        pqView * view = pqActiveView::instance().current();

        vtkSMRenderViewProxy* proxy = NULL;

        if (view)
        {
          proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
        }

        if (!proxy)
        {
          // no active view, or active view is not a render view.
          throw std::invalid_argument("Invalid vtkSMRenderViewProxy.");
        }

        // Get the aspect ratio of the renderer
        vtkRenderer* renderer = proxy->GetRenderer();
        if (!renderer)
        {
          throw std::invalid_argument("Invalid vtkRenderer.");
        }

        double aspectDimensions[2];
        renderer->GetAspect(aspectDimensions);
        double aspectRatio = aspectDimensions[0]/aspectDimensions[1];

        // Get the active camera 
        vtkCamera* camera = proxy->GetActiveCamera();

        if (!camera)
        {
          throw std::invalid_argument("Invalid vtkCamera.");
        }

        double planes[24];
        camera->GetFrustumPlanes(aspectRatio, planes);

        for (int k = 0; k < 4; ++k)
        {
          left[k] = planes[k];
          right[k] = planes[k + 4];

          bottom[k] = planes[k + 8];
          top[k] = planes[k + 12];

          near[k] = planes[k + 16];
          far[k] = planes[k + 20];
        }
        Mantid::VATES::ViewFrustum frustum(Mantid::VATES::LeftPlane(left[0], left[1], left[2], left[3]),
                                           Mantid::VATES::RightPlane(right[0], right[1], right[2], right[3]),
                                           Mantid::VATES::BottomPlane(bottom[0], bottom[1], bottom[2], bottom[3]),
                                           Mantid::VATES::TopPlane(top[0], top[1], top[2], top[3]),
                                           Mantid::VATES::FarPlane(far[0], far[1], far[2], far[3]),
                                           Mantid::VATES::NearPlane(near[0], near[1], near[2], near[3]));

        return frustum;
      }
    
      /**
      * Set the view onto a peak
      * @param xpos X position of the peak.
      * @param ypos Y position of the peak.
      * @param zpos Z position of the peak.
      * @param peakRadius The radius of the peak.
      */
      void CameraManager::setCameraToPeak(double xpos, double ypos, double zpos, double peakRadius)
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
          throw std::invalid_argument("Invalid vtkSMRenderViewProxy.");
        }

        // Get the active camera 
        vtkCamera* camera = proxy->GetActiveCamera();

        // Setup the focal point of the camera. we want this to be on the peak
        camera->SetFocalPoint(xpos, ypos, zpos);

        // Setup the position of the camera. We want this to be 
        double zposCamera = zpos + peakRadius;
        camera->SetPosition(xpos, ypos, zposCamera);
        camera->SetViewUp(0.0, 1.0, 0.0);

        view->forceRender();
      }
    }
  }
}