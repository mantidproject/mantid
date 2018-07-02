#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IExperimentView.h"
#include "IExperimentPresenter.h"
#include "ExperimentPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class ExperimentPresenterFactory {
public:
  std::unique_ptr<IExperimentPresenter> make(IExperimentView *view) {
    return std::make_unique<ExperimentPresenter>(view);
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
