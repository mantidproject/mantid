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
#include "GUI/Common/IFileHandler.h"
#include "GUI/Preview/ROIType.h"
#include "IExperimentPresenter.h"
#include "IExperimentView.h"
#include "LookupTableValidationError.h"
#include "Reduction/Experiment.h"
#include "Reduction/PreviewRow.h"
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class LookupRow;

class ExperimentValidationErrors {
public:
  explicit ExperimentValidationErrors(

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
      IExperimentView *view, Experiment experiment, double defaultsThetaTolerance, IFileHandler *fileHandler,
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
  void notifyPreviewApplyRequested(PreviewRow const &previewRow) override;
  void restoreDefaults() override;

  bool hasValidSettings() const noexcept override;

protected:
  std::unique_ptr<IExperimentOptionDefaults> m_experimentDefaults;

private:
  IBatchPresenter *m_mainPresenter;

  ExperimentValidationResult validateExperimentFromView();
  BackgroundSubtraction backgroundSubtractionFromView();
  PolarizationCorrections polarizationCorrectionsFromView();
  FloodCorrections floodCorrectionsFromView();
  std::optional<RangeInLambda> transmissionRunRangeFromView();
  std::string transmissionStitchParamsFromView();
  TransmissionStitchOptions transmissionStitchOptionsFromView();

  std::map<std::string, std::string> stitchParametersFromView();

  void showPolCorrFilePathValidity(std::string const &filePath);
  void showFloodFilePathValidity(std::string const &filePath);

  void updateModelFromView();
  void updateViewFromModel();

  void showValidationResult();
  void showLookupTableErrors(LookupTableValidationError const &errors);
  void showFullTableError(LookupCriteriaError const &tableError, int row, int column);

  void updateWidgetEnabledState();
  void updateSummationTypeEnabledState();
  void updateBackgroundSubtractionEnabledState();
  void updatePolarizationCorrectionEnabledState();
  void disablePolarizationEfficiencies();
  void updateFloodCorrectionEnabledState();
  void disableFloodCorrectionInputs();

  void updateLookupRowProcessingInstructions(PreviewRow const &previewRow, LookupRow &lookupRow, ROIType regionType);

  bool isProcessing() const;
  bool isAutoreducing() const;

  IExperimentView *m_view = nullptr;
  IFileHandler *m_fileHandler;
  Experiment m_model;
  double m_thetaTolerance = 0;
  ExperimentValidationResult m_validationResult;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
