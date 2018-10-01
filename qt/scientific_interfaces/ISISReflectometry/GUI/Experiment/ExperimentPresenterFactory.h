#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
#include "../../Reduction/Experiment.h"
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
    return std::make_unique<ExperimentPresenter>(view, makeModel(),
                                                 m_thetaTolerance);
  }

private:
  double m_thetaTolerance;

  Experiment makeModel() {
    // TODO get defaults from algorithm
    auto polarizationCorrections = PolarizationCorrections(0.0, 0.0, 0.0, 0.0);
    auto transmissionRunRange = RangeInLambda(0.0, 0.0);
    auto stitchParameters = std::map<std::string, std::string>();
    auto perThetaDefaults = std::vector<PerThetaDefaults>(
        {PerThetaDefaults(boost::none, std::pair<std::string, std::string>(),
                          boost::none, boost::none, ReductionOptionsMap())});
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                      SummationType::SumInLambda,
                      std::move(polarizationCorrections),
                      std::move(transmissionRunRange),
                      std::move(stitchParameters), std::move(perThetaDefaults));
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTERFACTORY_H
