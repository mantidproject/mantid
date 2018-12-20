// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

#include "../../DllConfig.h"
#include "../../Reduction/Experiment.h"
#include "../../ValidationResult.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include "IReflBatchPresenter.h"
#include "PerThetaDefaultsTableValidationError.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class ExperimentValidationErrors {
public:
  explicit ExperimentValidationErrors(
      // cppcheck-suppress passedByValue
      PerThetaDefaultsTableValidationError perThetaDefaultsErrors)
      : m_perThetaDefaultsErrors(std::move(perThetaDefaultsErrors)) {}

  PerThetaDefaultsTableValidationError const &perThetaValidationErrors() const {
    return m_perThetaDefaultsErrors;
  }

private:
  PerThetaDefaultsTableValidationError m_perThetaDefaultsErrors;
};

using ExperimentValidationResult =
    ValidationResult<Experiment, ExperimentValidationErrors>;

/** @class ExperimentPresenter

ExperimentPresenter is a presenter class for the widget 'Experiment' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentPresenter
    : public ExperimentViewSubscriber,
      public IExperimentPresenter {
public:
  // TODO Inject the Experiment model into the constructor.
  ExperimentPresenter(IExperimentView *view, Experiment experiment,
                      double defaultsThetaTolerance);

  Experiment const &experiment() const;

  void notifySettingsChanged() override;
  void notifySummationTypeChanged() override;
  void notifyNewPerAngleDefaultsRequested() override;
  void notifyRemovePerAngleDefaultsRequested(int index) override;
  void notifyPerAngleDefaultsChanged(int row, int column) override;

  void reductionPaused() override;
  void reductionResumed() override;

private:
  ExperimentValidationResult validateExperimentFromView();
  PolarizationCorrections polarizationCorrectionsFromView();
  FloodCorrections floodCorrectionsFromView();
  boost::optional<RangeInLambda> transmissionRunRangeFromView();
  std::map<std::string, std::string> stitchParametersFromView();

  ExperimentValidationResult updateModelFromView();

  void showValidationResult(ExperimentValidationResult const &result);
  void
  showPerThetaTableErrors(PerThetaDefaultsTableValidationError const &errors);

  IExperimentView *m_view;
  Experiment m_model;
  double m_thetaTolerance;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
