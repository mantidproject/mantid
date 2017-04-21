
//#include "MantidKernel/Logger.h"

#include "MantidVatesSimpleGuiViewWidgets/VisibleAxesColor.h"

#include <array>
#include <vector>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"

namespace Mantid {
// static logger
// Kernel::Logger g_log("VisibleAxesColor");

namespace Vates {
namespace SimpleGui {

namespace {
std::array<double, 3> getContrastingColor(const std::vector<double> &color) {
  double criteria =
      1. - (0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2]);

  if (criteria < 0.5)
    return {{0., 0., 0.}};
  else
    return {{1., 1., 1.}};
}

void safeSetProperty(vtkSMProxy *gridAxis,
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
  auto color = getContrastingColor(getBackgroundColor(view));
  safeSetProperty(view->getProxy(), {"OrientationAxesLabelColor"}, color);
}

void VisibleAxesColor::setGridAxesColor(pqRenderView *view) {
  auto color = getContrastingColor(getBackgroundColor(view));
  vtkSMProxy *gridAxes3DActor =
      vtkSMPropertyHelper(view->getProxy(), "AxesGrid", true).GetAsProxy();
  safeSetProperty(gridAxes3DActor,
                  {"XTitleColor", "YTitleColor", "ZTitleColor", "XLabelColor",
                   "YLabelColor", "ZLabelColor", "GridColor"},
                  color);
}

void VisibleAxesColor::setScalarBarColor(pqRenderView *view) {
  auto color = getContrastingColor(getBackgroundColor(view));

  // Update for all sources and all reps
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();

  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);
  // For all sources
  for (pqPipelineSource *source : sources) {
    const QList<pqDataRepresentation *> reps = source->getRepresentations(view);
    // For all representations
    for (pqDataRepresentation *rep : reps) {
      vtkSMProxy *ScalarBarProxy =
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
              rep->getLookupTableProxy(), view->getProxy());
      safeSetProperty(ScalarBarProxy, {"TitleColor", "LabelColor"}, color);
    }
  }
}

void VisibleAxesColor::observe(pqRenderView *view) {
  view->getViewProxy()
      ->GetProperty("Background")
      ->AddObserver(vtkCommand::ModifiedEvent, this,
                    &VisibleAxesColor::backgroundColorChangeCallback);
}

void VisibleAxesColor::backgroundColorChangeCallback(vtkObject *caller,
                                                     unsigned long, void *) {
  vtkSMDoubleVectorProperty *background =
      vtkSMDoubleVectorProperty::SafeDownCast(caller);
  int numberOfElements = background->GetNumberOfElements();
  double *elements = background->GetElements();
  std::vector<double> backgroundColor(elements, elements + numberOfElements);

  auto color = getContrastingColor(backgroundColor);

  // Update for all sources and all reps
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  pqView *view = pqActiveObjects::instance().activeView();

  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);
  // For all sources
  for (pqPipelineSource *source : sources) {
    const QList<pqDataRepresentation *> reps = source->getRepresentations(view);
    // For all representations
    for (pqDataRepresentation *rep : reps) {
      vtkSMProxy *ScalarBarProxy =
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
              rep->getLookupTableProxy(), view->getProxy());
      safeSetProperty(ScalarBarProxy, {"TitleColor", "LabelColor"}, color);
    }
  }
}

} // SimpleGui
} // Vates
} // Mantid
