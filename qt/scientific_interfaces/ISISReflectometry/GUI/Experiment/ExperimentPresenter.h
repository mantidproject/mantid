// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H

#include "Common/DllConfig.h"
#include "Common/ValidationResult.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include "PerThetaDefaultsTableValidationError.h"
#include "Reduction/Experiment.h"
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
  ExperimentPresenter(IExperimentView *view, Experiment experiment,
                      double defaultsThetaTolerance);

  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  Experiment const &experiment() const override;

  void notifySettingsChanged() override;
  void notifySummationTypeChanged() override;
  void notifyNewPerAngleDefaultsRequested() override;
  void notifyRemovePerAngleDefaultsRequested(int index) override;
  void notifyPerAngleDefaultsChanged(int row, int column) override;

  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionResumed() override;
  void instrumentChanged(
      std::string const &instrumentName,
      Mantid::Geometry::Instrument_const_sptr instrument) override;

private:
  IBatchPresenter *m_mainPresenter;
  ExperimentValidationResult validateExperimentFromView();
  PolarizationCorrections polarizationCorrectionsFromView();
  FloodCorrections floodCorrectionsFromView();
  boost::optional<RangeInLambda> transmissionRunRangeFromView();
  std::map<std::string, std::string> stitchParametersFromView();

  ExperimentValidationResult updateModelFromView();

  void showValidationResult(ExperimentValidationResult const &result);
  void
  showPerThetaTableErrors(PerThetaDefaultsTableValidationError const &errors);

  void updateWidgetEnabledState() const;
  bool isProcessing() const;
  bool isAutoreducing() const;

  IExperimentView *m_view;
  Experiment m_model;
  double m_thetaTolerance;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_EXPERIMENTPRESENTER_H
