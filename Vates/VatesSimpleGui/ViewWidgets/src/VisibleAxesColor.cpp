
//#include "MantidKernel/Logger.h"

#include "MantidVatesSimpleGuiViewWidgets/VisibleAxesColor.h"

#include <array>
#include <vector>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionProxy.h"

namespace Mantid {
// static logger
// Kernel::Logger g_log("VisibleAxesColor");

namespace Vates {
namespace SimpleGui {

namespace {

std::array<double, 3> GetContrastingColor(const std::vector<double> &color) {
  double criteria =
      1. - (0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2]);

  if (criteria < 0.5)
    return {{0., 0., 0.}};
  else
    return {{1., 1., 1.}};
}

void SafeSetProperty(vtkSMProxy *gridAxis, const char *pname,
                     const std::array<double, 3> &value) {
  if (gridAxis) {
    vtkSMProperty *prop = gridAxis->GetProperty(pname);
    if (prop) {
      vtkSMPropertyHelper helper(prop);
      helper.Set(value.data(), 3);
    }
  }
}
}

void VisibleAxesColor::setOrientationAxesLabelColor(
    pqRenderView *view, bool /*useCurrentBackgroundColor*/) {
  vtkSMProperty *prop = view->getProxy()->GetProperty("Background");
  vtkSMPropertyHelper helper(prop);
  auto backgroundColor = helper.GetDoubleArray();
  auto color = GetContrastingColor(backgroundColor);
  vtkSMPropertyHelper(view->getProxy(), "OrientationAxesLabelColor")
      .Set(color.data(), 3);
}

void VisibleAxesColor::setGridAxesColor(pqRenderView *view, bool /*on*/) {
  vtkSMProperty *prop = view->getProxy()->GetProperty("Background");
  vtkSMPropertyHelper helper(prop);
  auto backgroundColor = helper.GetDoubleArray();
  auto color = GetContrastingColor(backgroundColor);
  vtkSMProxy *gridAxes3DActor =
      vtkSMPropertyHelper(view->getProxy(), "AxesGrid", true).GetAsProxy();

  SafeSetProperty(gridAxes3DActor, "XTitleColor", color);
  gridAxes3DActor->UpdateProperty("XTitleColor");
  SafeSetProperty(gridAxes3DActor, "YTitleColor", color);
  gridAxes3DActor->UpdateProperty("YTitleColor");
  SafeSetProperty(gridAxes3DActor, "ZTitleColor", color);
  gridAxes3DActor->UpdateProperty("ZTitleColor");
  SafeSetProperty(gridAxes3DActor, "XLabelColor", color);
  gridAxes3DActor->UpdateProperty("XLabelColor");
  SafeSetProperty(gridAxes3DActor, "YLabelColor", color);
  gridAxes3DActor->UpdateProperty("YLabelColor");
  SafeSetProperty(gridAxes3DActor, "ZLabelColor", color);
  gridAxes3DActor->UpdateProperty("ZLabelColor");
  SafeSetProperty(gridAxes3DActor, "GridColor", color);
  gridAxes3DActor->UpdateProperty("GridColor");
}

void VisibleAxesColor::setScalarBarColor() {
  // Update for all sources and all reps
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  // For all sources
  foreach (pqPipelineSource *source, sources) {
    const QList<pqView *> views = source->getViews();
    // For all views
    foreach (pqView *view, views) {
      std::vector<double> backgroundRgb;
      if (view) {
        vtkSMProperty *prop = view->getProxy()->GetProperty("Background");
        vtkSMPropertyHelper helper(prop);
        backgroundRgb = helper.GetDoubleArray();
      }
      QList<pqDataRepresentation *> reps = source->getRepresentations(view);

      // For all representations
      foreach (pqDataRepresentation *rep, reps) {
        if (backgroundRgb.size() == 3) {
          auto color = GetContrastingColor(backgroundRgb);
          vtkSMProxy *ScalarBarProxy =
              vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
                  rep->getLookupTableProxy(), view->getProxy());

          if (ScalarBarProxy) {
            SafeSetProperty(ScalarBarProxy, "TitleColor", color);
            ScalarBarProxy->UpdateProperty("TitleColor");
            SafeSetProperty(ScalarBarProxy, "LabelColor", color);
            ScalarBarProxy->UpdateProperty("LabelColor");
          }
        }
      }
    }
  }
}

} // SimpleGui
} // Vates
} // Mantid
