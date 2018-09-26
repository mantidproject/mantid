#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "ExperimentPresenter.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class ExperimentPresenterFactory {
public:
  explicit ExperimentPresenterFactory(double thetaTolerance)
      : m_thetaTolerance(thetaTolerance) {}

  std::unique_ptr<IExperimentPresenter> make(IExperimentView *view) {
    return std::make_unique<ExperimentPresenter>(view, m_thetaTolerance);
  }

private:
  double m_thetaTolerance;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
