#include "ExperimentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view)
    : m_view(view), m_model() {
  m_view->subscribe(this);
  notifySettingsChanged();
}

void ExperimentPresenter::notifyNewPerAngleDefaultsRequested() {
  m_view->addPerThetaDefaultsRow();
  notifySettingsChanged();
}

void ExperimentPresenter::notifyRemovePerAngleDefaultsRequested(int index) {
  m_view->removePerThetaDefaultsRow(index);
  notifySettingsChanged();
}

class InvalidDefaultsError {
public:
  InvalidDefaultsError(int row, std::vector<int> invalidColumns);
  std::vector<int> const &invalidColumns() const;
  int row() const;

private:
  std::vector<int> m_invalidColumns;
  int m_row;
};

InvalidDefaultsError::InvalidDefaultsError(int row,
                                           std::vector<int> invalidColumns)
    : m_invalidColumns(invalidColumns), m_row(row) {}

std::vector<int> const &InvalidDefaultsError::invalidColumns() const {
  return m_invalidColumns;
}

int InvalidDefaultsError::row() const { return m_row; }

class PerAngleDefaultsValidationResult {
public:
  PerAngleDefaultsValidationResult(
      std::vector<PerThetaDefaults> defaults,
      std::vector<InvalidDefaultsError> validationErrors, bool hasUniqueThetas);

  bool hasUniqueThetas() const;
  bool isValid() const;
  std::vector<PerThetaDefaults> const &defaults() const;
  std::vector<InvalidDefaultsError> const &errors() const;

  std::vector<PerThetaDefaults> m_defaults;
  std::vector<InvalidDefaultsError> m_validationErrors;
  bool m_hasUniqueThetas;
};

PerAngleDefaultsValidationResult::PerAngleDefaultsValidationResult(
    std::vector<PerThetaDefaults> defaults,
    std::vector<InvalidDefaultsError> validationErrors, bool hasUniqueThetas)
    : m_defaults(std::move(defaults)),
      m_validationErrors(std::move(validationErrors)),
      m_hasUniqueThetas(hasUniqueThetas) {}

bool PerAngleDefaultsValidationResult::isValid() const {
  return hasUniqueThetas() && m_validationErrors.empty();
}

bool PerAngleDefaultsValidationResult::hasUniqueThetas() const {
  return m_hasUniqueThetas;
}

std::vector<PerThetaDefaults> const &
PerAngleDefaultsValidationResult::defaults() const {
  return m_defaults;
}

std::vector<InvalidDefaultsError> const &
PerAngleDefaultsValidationResult::errors() const {
  return m_validationErrors;
}

PerAngleDefaultsValidationResult validatePerThetaDefaultsFromView(
    std::vector<std::array<std::string, 8>> const &perThetaDefaultsContent,
    double thetaTolerance) {
  auto defaults = std::vector<PerThetaDefaults>();
  auto validationErrors = std::vector<InvalidDefaultsError>();

  auto row = 0;
  for (auto const &rowTemplateContent : perThetaDefaultsContent) {
    auto rowValidationResult = validatePerThetaDefaults(rowTemplateContent);
    if (rowValidationResult.isValid())
      defaults.emplace_back(rowValidationResult.validRowElseNone().get());
    else
      validationErrors.emplace_back(row, rowValidationResult.invalidColumns());
    row++;
  }

  std::cout << "\n";
  std::cout << "There were " << validationErrors.size()
            << " validation errors.\n";
  std::cout << "There were " << defaults.size() << " ok rows." << std::endl;

  auto const hasUniqueThetas =
      Experiment::thetaValuesAreUnique(defaults, thetaTolerance);
  return PerAngleDefaultsValidationResult(
      std::move(defaults), std::move(validationErrors), hasUniqueThetas);
}

void ExperimentPresenter::notifySettingsChanged() {
  auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
  auto const reductionType =
      reductionTypeFromString(m_view->getReductionType());
  auto const summationType =
      summationTypeFromString(m_view->getSummationType());
  auto const cRho = m_view->getCRho();
  auto const cAlpha = m_view->getCAlpha();
  auto const cAp = m_view->getCAp();
  auto const cPp = m_view->getCPp();
  auto polarizationCorrections =
      PolarizationCorrections(cRho, cAlpha, cAp, cPp);
  auto transmissionRunRange =
      RangeInLambda(m_view->getTransmissionStartOverlap(),
                    m_view->getTransmissionEndOverlap());
  auto stitchParameters = m_view->getStitchOptions();

  auto perThetaDefaultsValidationResult =
      validatePerThetaDefaultsFromView(m_view->getPerAngleOptions(), 0.01);

  if (perThetaDefaultsValidationResult.isValid()) {
    m_model = Experiment(analysisMode, reductionType, summationType,
                         polarizationCorrections, transmissionRunRange,
                         stitchParameters,
                         perThetaDefaultsValidationResult.defaults());
    m_view->showAllPerAngleOptionsAsValid();
  } else {
    m_view->showAllPerAngleOptionsAsValid();
    for (auto const &validationError :
         perThetaDefaultsValidationResult.errors())
      for (auto const &column : validationError.invalidColumns()) {
        std::cout << "Setting row " << validationError.row() << " column "
                  << column << " as invalid." << std::endl;
        m_view->showPerAngleOptionsAsInvalid(validationError.row(), column);
      }

    if (!perThetaDefaultsValidationResult.hasUniqueThetas())
      m_view->showPerAngleThetasNonUnique(0.01);
  }
}

void ExperimentPresenter::notifySummationTypeChanged() {
  if (m_model.get().summationType() == SummationType::SumInQ)
    m_view->enableReductionType();
  else
    m_view->disableReductionType();
}

Experiment const &ExperimentPresenter::experiment() const {
  return m_model.get();
}
}
}
