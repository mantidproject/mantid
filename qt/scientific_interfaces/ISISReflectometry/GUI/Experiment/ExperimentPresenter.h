// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "Common/ValidationResult.h"
#include "ExperimentOptionDefaults.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include "LookupTableValidationError.h"
#include "Reduction/Experiment.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class ExperimentValidationErrors {
public:
  explicit ExperimentValidationErrors(
      // cppcheck-suppress passedByValue
      LookupTableValidationError lookupTableErrors)
      : m_lookupTableErrors(std::move(lookupTableErrors)) {}

  LookupTableValidationError const &lookupTableValidationErrors() const { return m_lookupTableErrors; }

private:
  LookupTableValidationError m_lookupTableErrors;
};

using ExperimentValidationResult = ValidationResult<Experiment, ExperimentValidationErrors>;

/** @class ExperimentPresenter

ExperimentPresenter is a presenter class for the widget 'Experiment' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentPresenter : public ExperimentViewSubscriber,
                                                           public IExperimentPresenter {
public:
  ExperimentPresenter(
      IExperimentView *view, Experiment experiment, double defaultsThetaTolerance,
      std::unique_ptr<IExperimentOptionDefaults> experimentDefaults = std::make_unique<ExperimentOptionDefaults>());

  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  Experiment const &experiment() const override;

  void notifySettingsChanged() override;
  void notifyRestoreDefaultsRequested() override;
  void notifySummationTypeChanged() override;
  void notifyNewLookupRowRequested() override;
  void notifyRemoveLookupRowRequested(int index) override;
  void notifyLookupRowChanged(int row, int column) override;

  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void restoreDefaults() override;

protected:
  std::unique_ptr<IExperimentOptionDefaults> m_experimentDefaults;

private:
  IBatchPresenter *m_mainPresenter;

  ExperimentValidationResult validateExperimentFromView();
  BackgroundSubtraction backgroundSubtractionFromView();
  PolarizationCorrections polarizationCorrectionsFromView();
  FloodCorrections floodCorrectionsFromView();
  boost::optional<RangeInLambda> transmissionRunRangeFromView();
  std::string transmissionStitchParamsFromView();
  TransmissionStitchOptions transmissionStitchOptionsFromView();

  std::map<std::string, std::string> stitchParametersFromView();

  ExperimentValidationResult updateModelFromView();
  void updateViewFromModel();

  void showValidationResult(ExperimentValidationResult const &result);
  void showLookupTableErrors(LookupTableValidationError const &errors);

  void updateWidgetEnabledState();
  void updateSummationTypeEnabledState();
  void updateBackgroundSubtractionEnabledState();
  void updatePolarizationCorrectionEnabledState();
  void updateFloodCorrectionEnabledState();

  bool isProcessing() const;
  bool isAutoreducing() const;

  IExperimentView *m_view;
  Experiment m_model;
  double m_thetaTolerance;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
