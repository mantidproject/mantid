
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

void SafeSetProperty(vtkSMProxy *gridAxis,
                     std::initializer_list<const char *> pnames,
                     const std::array<double, 3> &value) {
  if (gridAxis) {
    for (auto pname : pnames) {
      vtkSMProperty *prop = gridAxis->GetProperty(pname);
      if (prop) {
        vtkSMPropertyHelper helper(prop);
        helper.Set(value.data(), 3);
        gridAxis->UpdateProperty(pname);
      }
    }
  }
}

std::vector<double> getBackgroundColor(pqRenderView *view) {
  vtkSMProperty *prop = view->getProxy()->GetProperty("Background");
  return vtkSMPropertyHelper(prop).GetDoubleArray();
}
}

void VisibleAxesColor::setOrientationAxesLabelColor(pqRenderView *view) {
  auto color = GetContrastingColor(getBackgroundColor(view));
  SafeSetProperty(view->getProxy(), {"OrientationAxesLabelColor"}, color);
}

void VisibleAxesColor::setGridAxesColor(pqRenderView *view) {
  auto color = GetContrastingColor(getBackgroundColor(view));
  vtkSMProxy *gridAxes3DActor =
      vtkSMPropertyHelper(view->getProxy(), "AxesGrid", true).GetAsProxy();
  SafeSetProperty(gridAxes3DActor,
                  {"XTitleColor", "YTitleColor", "ZTitleColor", "XLabelColor",
                   "YLabelColor", "ZLabelColor", "GridColor"},
                  color);
}

void VisibleAxesColor::setScalarBarColor(pqRenderView *view) {

  auto color = GetContrastingColor(getBackgroundColor(view));

  // Update for all sources and all reps
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  // For all sources
  foreach (pqPipelineSource *source, sources) {
    QList<pqDataRepresentation *> reps = source->getRepresentations(view);
    // For all representations
    foreach (pqDataRepresentation *rep, reps) {
      vtkSMProxy *ScalarBarProxy =
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
              rep->getLookupTableProxy(), view->getProxy());
      if (ScalarBarProxy) {
        SafeSetProperty(ScalarBarProxy, {"TitleColor", "LabelColor"}, color);
      }
    }
  }
}

} // SimpleGui
} // Vates
} // Mantid
