#include "IndirectFitAnalysisTab.h"
#include "ui_ConvFit.h"
#include "ui_IqtFit.h"
#include "ui_JumpFit.h"
#include "ui_MSDFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/make_unique.h"

#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QString>
#include <QtCore>

#include <algorithm>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

void updateParameters(
    IFunction_sptr function,
    const std::unordered_map<std::string, ParameterValue> &parameters) {
  for (auto i = 0u; i < function->nParams(); ++i) {
    auto value = parameters.find(function->parameterName(i));
    if (value != parameters.end()) {
      function->setParameter(i, value->second.value);
      if (value->second.error)
        function->setError(i, *value->second.error);
    }
  }
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor.
 *
 * @param parent :: the parent widget (an IndirectDataAnalysis object).
 */
IndirectFitAnalysisTab::IndirectFitAnalysisTab(IndirectFittingModel *model,
                                               QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_fittingModel(model) {}

void IndirectFitAnalysisTab::setup() {
  setupFitTab();
  updateResultOptions();

  connect(m_dataPresenter.get(),
          SIGNAL(startXChanged(double, std::size_t, std::size_t)), this,
          SLOT(tableStartXChanged(double, std::size_t, std::size_t)));
  connect(m_dataPresenter.get(),
          SIGNAL(endXChanged(double, std::size_t, std::size_t)), this,
          SLOT(tableEndXChanged(double, std::size_t, std::size_t)));
  connect(m_dataPresenter.get(),
          SIGNAL(excludeRegionChanged(const std::string &, std::size_t,
                                      std::size_t)),
          this, SLOT(tableExcludeChanged(const std::string &, std::size_t,
                                         std::size_t)));
  connect(m_dataPresenter.get(), SIGNAL(singleResolutionLoaded()), this,
          SLOT(setModelFitFunction()));

  connect(m_fitPropertyBrowser, SIGNAL(fitScheduled()), this,
          SLOT(singleFit()));
  connect(m_fitPropertyBrowser, SIGNAL(sequentialFitScheduled()), this,
          SLOT(executeFit()));

  connect(m_fitPropertyBrowser, SIGNAL(startXChanged(double)), this,
          SLOT(setModelStartX(double)));
  connect(m_fitPropertyBrowser, SIGNAL(endXChanged(double)), this,
          SLOT(setModelEndX(double)));

  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)));

  connect(m_fitPropertyBrowser,
          SIGNAL(customBoolChanged(const QString &, bool)), this,
          SIGNAL(customBoolChanged(const QString &, bool)));

  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(setModelFitFunction()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SIGNAL(functionChanged()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updatePlotOptions()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updateResultOptions()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(updateParameterValues()));
  connect(m_plotPresenter.get(), SIGNAL(selectedFitDataChanged(std::size_t)),
          this, SLOT(updateParameterValues()));
  connect(m_plotPresenter.get(), SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(updateParameterValues()));

  connect(m_plotPresenter.get(),
          SIGNAL(fitSingleSpectrum(std::size_t, std::size_t)), this,
          SLOT(singleFit(std::size_t, std::size_t)));
  connect(m_plotPresenter.get(),
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), this,
          SLOT(updateResultOptions()));

  connectDataAndSpectrumPresenters();
  connectDataAndPlotPresenters();
  connectDataAndFitBrowserPresenters();
  connectSpectrumAndPlotPresenters();
  connectFitBrowserAndPlotPresenter();
}

void IndirectFitAnalysisTab::connectDataAndPlotPresenters() {
  connect(m_dataPresenter.get(), SIGNAL(multipleDataViewSelected()),
          m_plotPresenter.get(), SLOT(showMultipleDataSelection()));
  connect(m_dataPresenter.get(), SIGNAL(singleDataViewSelected()),
          m_plotPresenter.get(), SLOT(hideMultipleDataSelection()));

  connect(m_dataPresenter.get(), SIGNAL(dataAdded()), m_plotPresenter.get(),
          SLOT(appendLastDataToSelection()));
  connect(m_dataPresenter.get(), SIGNAL(dataRemoved()), m_plotPresenter.get(),
          SLOT(updateDataSelection()));

  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), m_plotPresenter.get(),
          SLOT(updateAvailableSpectra()));
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), m_plotPresenter.get(),
          SLOT(updatePlots()));
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), m_plotPresenter.get(),
          SLOT(updateGuess()));

  connect(m_dataPresenter.get(), SIGNAL(singleResolutionLoaded()),
          m_plotPresenter.get(), SLOT(updatePlots()));
  connect(m_dataPresenter.get(), SIGNAL(singleResolutionLoaded()),
          m_plotPresenter.get(), SLOT(updateGuess()));

  connect(m_plotPresenter.get(), SIGNAL(startXChanged(double)), this,
          SLOT(setDataTableStartX(double)));
  connect(m_plotPresenter.get(), SIGNAL(endXChanged(double)), this,
          SLOT(setDataTableEndX(double)));
}

void IndirectFitAnalysisTab::connectSpectrumAndPlotPresenters() {
  connect(m_plotPresenter.get(), SIGNAL(selectedFitDataChanged(std::size_t)),
          m_spectrumPresenter.get(), SLOT(setActiveModelIndex(std::size_t)));
  connect(m_plotPresenter.get(), SIGNAL(noFitDataSelected()),
          m_spectrumPresenter.get(), SLOT(disableView()));
  connect(m_spectrumPresenter.get(), SIGNAL(spectraChanged(std::size_t)),
          m_plotPresenter.get(), SLOT(updateSelectedDataName()));
  connect(m_spectrumPresenter.get(), SIGNAL(spectraChanged(std::size_t)),
          m_plotPresenter.get(), SLOT(updateAvailableSpectra()));
}

void IndirectFitAnalysisTab::connectFitBrowserAndPlotPresenter() {
  connect(m_plotPresenter.get(), SIGNAL(selectedFitDataChanged(std::size_t)),
          this, SLOT(setBrowserWorkspace(std::size_t)));
  connect(m_plotPresenter.get(), SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(setBrowserWorkspaceIndex(std::size_t)));

  connect(m_fitPropertyBrowser, SIGNAL(startXChanged(double)),
          m_plotPresenter.get(), SLOT(setStartX(double)));
  connect(m_fitPropertyBrowser, SIGNAL(endXChanged(double)),
          m_plotPresenter.get(), SLOT(setEndX(double)));

  connect(m_plotPresenter.get(), SIGNAL(startXChanged(double)), this,
          SLOT(setBrowserStartX(double)));
  connect(m_plotPresenter.get(), SIGNAL(endXChanged(double)), this,
          SLOT(setBrowserEndX(double)));
  connect(m_plotPresenter.get(), SIGNAL(fwhmChanged(double)), this,
          SLOT(updateFitBrowserParameterValues()));
  connect(m_plotPresenter.get(), SIGNAL(backgroundChanged(double)), this,
          SLOT(updateFitBrowserParameterValues()));

  connect(m_fitPropertyBrowser, SIGNAL(xRangeChanged(double, double)),
          m_plotPresenter.get(), SLOT(updateGuess()));
  connect(m_plotPresenter.get(), SIGNAL(fwhmChanged(double)),
          m_plotPresenter.get(), SLOT(updateGuess()));
  connect(m_plotPresenter.get(), SIGNAL(backgroundChanged(double)),
          m_plotPresenter.get(), SLOT(updateGuess()));

  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)),
          m_plotPresenter.get(), SLOT(updateRangeSelectors()));
  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)),
          m_plotPresenter.get(), SLOT(updateGuess()));

  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()),
          m_plotPresenter.get(), SLOT(updatePlots()));
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()),
          m_plotPresenter.get(), SLOT(updateGuess()));

  connect(m_fitPropertyBrowser, SIGNAL(plotGuess()), m_plotPresenter.get(),
          SLOT(enablePlotGuessInSeparateWindow()));
}

void IndirectFitAnalysisTab::connectDataAndSpectrumPresenters() {
  connect(m_dataPresenter.get(), SIGNAL(singleDataViewSelected()),
          m_spectrumPresenter.get(), SLOT(setActiveIndexToZero()));
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()),
          m_spectrumPresenter.get(), SLOT(updateSpectra()));
  connect(m_spectrumPresenter.get(), SIGNAL(spectraChanged(std::size_t)),
          m_dataPresenter.get(), SLOT(updateSpectraInTable(std::size_t)));
  connect(m_spectrumPresenter.get(), SIGNAL(maskChanged(const std::string &)),
          this, SLOT(setDataTableExclude(const std::string &)));
}

void IndirectFitAnalysisTab::connectDataAndFitBrowserPresenters() {
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), this,
          SLOT(updateBrowserFittingRange()));
  connect(m_fitPropertyBrowser, SIGNAL(startXChanged(double)), this,
          SLOT(setDataTableStartX(double)));
  connect(m_fitPropertyBrowser, SIGNAL(endXChanged(double)), this,
          SLOT(setDataTableEndX(double)));
}

void IndirectFitAnalysisTab::setFitDataPresenter(
    IndirectFitDataPresenter *presenter) {
  m_dataPresenter = std::unique_ptr<IndirectFitDataPresenter>(presenter);
}

void IndirectFitAnalysisTab::setPlotView(IndirectFitPlotView *view) {
  m_plotPresenter = Mantid::Kernel::make_unique<IndirectFitPlotPresenter>(
      m_fittingModel.get(), view);
}

void IndirectFitAnalysisTab::setSpectrumSelectionView(
    IndirectSpectrumSelectionView *view) {
  m_spectrumPresenter =
      Mantid::Kernel::make_unique<IndirectSpectrumSelectionPresenter>(
          m_fittingModel.get(), view);
}

void IndirectFitAnalysisTab::setFitPropertyBrowser(
    MantidWidgets::IndirectFitPropertyBrowser *browser) {
  browser->init();
  m_fitPropertyBrowser = browser;
}

void IndirectFitAnalysisTab::loadSettings(const QSettings &settings) {
  m_dataPresenter->loadSettings(settings);
}

void IndirectFitAnalysisTab::setSampleWSSuffices(const QStringList &suffices) {
  m_dataPresenter->setSampleWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setSampleFBSuffices(const QStringList &suffices) {
  m_dataPresenter->setSampleFBSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionWSSuffices(
    const QStringList &suffices) {
  m_dataPresenter->setResolutionWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionFBSuffices(
    const QStringList &suffices) {
  m_dataPresenter->setResolutionFBSuffices(suffices);
}

std::size_t IndirectFitAnalysisTab::getSelectedDataIndex() const {
  return m_plotPresenter->getSelectedDataIndex();
}

std::size_t IndirectFitAnalysisTab::getSelectedSpectrum() const {
  return m_plotPresenter->getSelectedSpectrum();
}

bool IndirectFitAnalysisTab::isRangeCurrentlySelected(
    std::size_t dataIndex, std::size_t spectrum) const {
  return FittingMode::SEQUENTIAL == m_fittingModel->getFittingMode() ||
         m_plotPresenter->isCurrentlySelected(dataIndex, spectrum);
}

IndirectFittingModel *IndirectFitAnalysisTab::fittingModel() const {
  return m_fittingModel.get();
}

/**
 * @return  The fit type selected in the custom functions combo box, in the fit
 *          property browser.
 */
QString IndirectFitAnalysisTab::selectedFitType() const {
  return m_fitPropertyBrowser->selectedFitType();
}

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitAnalysisTab::numberOfCustomFunctions(
    const std::string &functionName) const {
  return m_fitPropertyBrowser->numberOfCustomFunctions(functionName);
}

void IndirectFitAnalysisTab::setModelFitFunction() {
  try {
    m_fittingModel->setFitFunction(m_fitPropertyBrowser->getFittingFunction());
  } catch (const std::out_of_range &) {
    m_fittingModel->setFitFunction(m_fitPropertyBrowser->compositeFunction());
  }
}

void IndirectFitAnalysisTab::setModelStartX(double startX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->numberOfWorkspaces() > dataIndex) {
    m_fittingModel->setStartX(startX, dataIndex, getSelectedSpectrum());
  } else {
    setBrowserStartX(0);
    setBrowserEndX(0);
  }
}

void IndirectFitAnalysisTab::setModelEndX(double endX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->numberOfWorkspaces() > dataIndex) {
    m_fittingModel->setEndX(endX, dataIndex, getSelectedSpectrum());
  } else {
    setBrowserStartX(0);
    setBrowserEndX(0);
  }
}

void IndirectFitAnalysisTab::setDataTableStartX(double startX) {
  m_dataPresenter->setStartX(startX, m_plotPresenter->getSelectedDataIndex(),
                             m_plotPresenter->getSelectedSpectrumIndex());
}

void IndirectFitAnalysisTab::setDataTableEndX(double endX) {
  m_dataPresenter->setEndX(endX, m_plotPresenter->getSelectedDataIndex(),
                           m_plotPresenter->getSelectedSpectrumIndex());
}

void IndirectFitAnalysisTab::setDataTableExclude(const std::string &exclude) {
  m_dataPresenter->setExclude(exclude, m_plotPresenter->getSelectedDataIndex(),
                              m_plotPresenter->getSelectedSpectrumIndex());
}

void IndirectFitAnalysisTab::setBrowserStartX(double startX) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_fitPropertyBrowser);
  m_fitPropertyBrowser->setStartX(startX);
}

void IndirectFitAnalysisTab::setBrowserEndX(double endX) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_fitPropertyBrowser);
  m_fitPropertyBrowser->setEndX(endX);
}

void IndirectFitAnalysisTab::updateBrowserFittingRange() {
  const auto range = m_fittingModel->getFittingRange(getSelectedDataIndex(),
                                                     getSelectedSpectrum());
  setBrowserStartX(range.first);
  setBrowserEndX(range.second);
}

void IndirectFitAnalysisTab::setBrowserWorkspace(std::size_t dataIndex) {
  const auto name = m_fittingModel->getWorkspace(dataIndex)->getName();
  m_fitPropertyBrowser->setWorkspaceName(QString::fromStdString(name));
}

void IndirectFitAnalysisTab::setBrowserWorkspaceIndex(std::size_t spectrum) {
  m_fitPropertyBrowser->setWorkspaceIndex(boost::numeric_cast<int>(spectrum));
}

void IndirectFitAnalysisTab::tableStartXChanged(double startX,
                                                std::size_t dataIndex,
                                                std::size_t spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum)) {
    m_plotPresenter->setStartX(startX);
    setBrowserStartX(startX);
  }
}

void IndirectFitAnalysisTab::tableEndXChanged(double endX,
                                              std::size_t dataIndex,
                                              std::size_t spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum)) {
    m_plotPresenter->setEndX(endX);
    setBrowserEndX(endX);
  }
}

void IndirectFitAnalysisTab::tableExcludeChanged(const std::string &,
                                                 std::size_t dataIndex,
                                                 std::size_t spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum))
    m_spectrumPresenter->displayBinMask();
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitAnalysisTab::setConvolveMembers(bool convolveMembers) {
  m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
}

/**
 * Updates the ties displayed in the fit property browser, using
 * the set fitting function.
 */
void IndirectFitAnalysisTab::updateTies() {
  m_fitPropertyBrowser->updateTies();
}

/**
 * Sets whether the custom setting with the specified name is enabled.
 *
 * @param settingName The name of the custom setting.
 * @param enabled     True if custom setting should be enabled, false otherwise.
 */
void IndirectFitAnalysisTab::setCustomSettingEnabled(const QString &customName,
                                                     bool enabled) {
  m_fitPropertyBrowser->setCustomSettingEnabled(customName, enabled);
}

/**
 * Sets the value of the parameter with the specified name, in the function with
 * the specified name.
 *
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter to set.
 * @param value         The value to set.
 */
void IndirectFitAnalysisTab::setParameterValue(const std::string &functionName,
                                               const std::string &parameterName,
                                               double value) {
  m_fitPropertyBrowser->setParameterValue(functionName, parameterName, value);
}

/**
 * Sets the default peak type for the indirect property browser.
 *
 * @param function  The name of the default peak function to set.
 */
void IndirectFitAnalysisTab::setDefaultPeakType(const std::string &function) {
  m_fitPropertyBrowser->setDefaultPeakType(function);
}

/**
 * Adds a check-box with the specified name, to the fit property browser, which
 * when checked adds the specified functions to the mode and when unchecked,
 * removes them.
 *
 * @param groupName     The name/label of the check-box to add.
 * @param functions     The functions to be added when the check-box is checked.
 * @param defaultValue  The default value of the check-box.
 */
void IndirectFitAnalysisTab::addCheckBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    bool defaultValue) {
  m_fitPropertyBrowser->addCheckBoxFunctionGroup(groupName, functions,
                                                 defaultValue);
}

/**
 * Adds a number spinner with the specified name, to the fit property browser,
 * which specifies how many multiples of the specified functions should be added
 * to the model.
 *
 * @param groupName     The name/label of the spinner to add.
 * @param functions     The functions to be added.
 * @param minimum       The minimum value of the spinner.
 * @param maximum       The maximum value of the spinner.
 * @param defaultValue  The default value of the spinner.
 */
void IndirectFitAnalysisTab::addSpinnerFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    int minimum, int maximum, int defaultValue) {
  m_fitPropertyBrowser->addSpinnerFunctionGroup(groupName, functions, minimum,
                                                maximum, defaultValue);
}

/**
 * Adds an option with the specified name, to the fit type combo-box in the fit
 * property browser, which adds the specified functions to the model.
 *
 * @param groupName The name of the option to be added to the fit type
 *                  combo-box.
 * @param functions The functions added by the option.
 */
void IndirectFitAnalysisTab::addComboBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  m_fitPropertyBrowser->addComboBoxFunctionGroup(groupName, functions);
}

/**
 * Sets the available background options in this fit analysis tab.
 *
 * @param backgrounds A list of the available backgrounds.
 */
void IndirectFitAnalysisTab::setBackgroundOptions(
    const QStringList &backgrounds) {
  m_fitPropertyBrowser->setBackgroundOptions(backgrounds);
}

/**
 * @param settingKey  The key of the boolean setting whose value to retrieve.
 * @return            The value of the boolean setting with the specified key.
 */
bool IndirectFitAnalysisTab::boolSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->boolSettingValue(settingKey);
}

/**
 * Sets the value of the custom boolean setting, with the specified key, to the
 * specified value.
 *
 * @param settingKey  The key of the custom boolean setting.
 * @param value       The value to set the boolean custom setting to.
 */
void IndirectFitAnalysisTab::setCustomBoolSetting(const QString &settingKey,
                                                  bool value) {
  m_fitPropertyBrowser->setCustomBoolSetting(settingKey, value);
}

/**
 * @param settingKey  The key of the integer setting whose value to retrieve.
 * @return            The value of the integer setting with the specified key.
 */
int IndirectFitAnalysisTab::intSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->intSettingValue(settingKey);
}

/**
 * @param settingKey  The key of the double setting whose value to retrieve.
 * @return            The value of the double setting with the specified key.
 */
double
IndirectFitAnalysisTab::doubleSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->doubleSettingValue(settingKey);
}

/**
 * @param settingKey  The key of the enum setting whose value to retrieve.
 * @return            The value of the enum setting with the specified key.
 */
QString
IndirectFitAnalysisTab::enumSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->enumSettingValue(settingKey);
}

/**
 * Adds a boolean custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the boolean setting to add.
 * @param settingName   The display name of the boolean setting to add.
 * @param defaultValue  The default value of the boolean setting.
 */
void IndirectFitAnalysisTab::addBoolCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  bool defaultValue) {
  m_fitPropertyBrowser->addBoolCustomSetting(settingKey, settingName,
                                             defaultValue);
}

/**
 * Adds a double custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the double setting to add.
 * @param settingName   The display name of the double setting to add.
 * @param defaultValue  The default value of the double setting.
 */
void IndirectFitAnalysisTab::addDoubleCustomSetting(const QString &settingKey,
                                                    const QString &settingName,
                                                    double defaultValue) {
  m_fitPropertyBrowser->addDoubleCustomSetting(settingKey, settingName,
                                               defaultValue);
}

/**
 * Adds an integer custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the integer setting to add.
 * @param settingName   The display name of the integer setting to add.
 * @param defaultValue  The default value of the integer setting.
 */
void IndirectFitAnalysisTab::addIntCustomSetting(const QString &settingKey,
                                                 const QString &settingName,
                                                 int defaultValue) {
  m_fitPropertyBrowser->addIntCustomSetting(settingKey, settingName,
                                            defaultValue);
}

/**
 * Adds an enum custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the enum setting to add.
 * @param settingName   The display name of the enum setting to add.
 * @param defaultValue  The default value of the enum setting.
 */
void IndirectFitAnalysisTab::addEnumCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  const QStringList &options) {
  m_fitPropertyBrowser->addEnumCustomSetting(settingKey, settingName, options);
}

/**
 * Adds an optional double custom setting, with the specified key and display
 * name.
 *
 * @param settingKey    The key of the optional double setting to add.
 * @param settingName   The display name of the optional double setting to add.
 * @param optionKey     The key of the setting specifying whether to use this
 *                      this optional setting.
 * @param optionName    The display name of the setting specifying whether to
 *                      use this optional setting.
 * @param defaultValue  The default value of the optional double setting.
 */
void IndirectFitAnalysisTab::addOptionalDoubleSetting(
    const QString &settingKey, const QString &settingName,
    const QString &optionKey, const QString &optionName, bool enabled,
    double defaultValue) {
  m_fitPropertyBrowser->addOptionalDoubleSetting(
      settingKey, settingName, optionKey, optionName, enabled, defaultValue);
}

/**
 * Sets whether a setting with a specified key affects the fitting function.
 *
 * @param settingKey      The key of the setting.
 * @param changesFunction Boolean specifying whether the setting affects the
 *                        fitting function.
 */
void IndirectFitAnalysisTab::setCustomSettingChangesFunction(
    const QString &settingKey, bool changesFunction) {
  m_fitPropertyBrowser->setCustomSettingChangesFunction(settingKey,
                                                        changesFunction);
}

void IndirectFitAnalysisTab::updateFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(updateFitOutput(bool)));

  if (error)
    m_fittingModel->cleanFailedRun(m_fittingAlgorithm);
  else
    m_fittingModel->addOutput(m_fittingAlgorithm);
}

void IndirectFitAnalysisTab::updateSingleFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(updateSingleFitOutput(bool)));

  if (error)
    m_fittingModel->cleanFailedSingleRun(m_fittingAlgorithm, 0);
  else
    m_fittingModel->addSingleFitOutput(m_fittingAlgorithm, 0);
}

/*
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(bool error) {
  setSaveResultEnabled(!error);
  setPlotResultEnabled(!error);
  m_spectrumPresenter->enableView();
  m_plotPresenter->updatePlots();
  updateParameterValues();
  updatePlotOptions();

  connect(m_fitPropertyBrowser,
          SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(updateGuessPlots()));
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(fitAlgorithmComplete(bool)));
}

void IndirectFitAnalysisTab::updateFitBrowserParameterValues() {
  MantidQt::API::SignalBlocker<QObject> blocker(m_fitPropertyBrowser);
  m_fitPropertyBrowser->updateParameters();
}

/**
 * Updates the parameter values and errors in the fit property browser.
 */
void IndirectFitAnalysisTab::updateParameterValues() {
  updateParameterValues(m_fittingModel->getParameterValues(
      getSelectedDataIndex(), getSelectedSpectrum()));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 *
 * @param parameters  The parameter values to update the browser with.
 */
void IndirectFitAnalysisTab::updateParameterValues(
    const std::unordered_map<std::string, ParameterValue> &parameters) {
  try {
    auto fitFunction = m_fitPropertyBrowser->getFittingFunction();
    updateParameters(fitFunction, parameters);

    MantidQt::API::SignalBlocker<QObject> blocker(m_fitPropertyBrowser);
    m_fitPropertyBrowser->updateParameters();

    if (m_fittingModel->isPreviouslyFit(getSelectedDataIndex(),
                                        getSelectedSpectrum()))
      m_fitPropertyBrowser->updateErrors();
    else
      m_fitPropertyBrowser->clearErrors();
  } catch (const std::out_of_range &) {
  }
}

/*
 * Saves the result workspace in the default save directory.
 */
void IndirectFitAnalysisTab::saveResult() { m_fittingModel->saveResult(); }

/*
 * Plots the result workspace with the specified name, using the specified
 * plot type. Plot type can either be 'None', 'All' or the name of a
 * parameter. In the case of 'None', nothing will be plotted. In the case of
 * 'All', everything will be plotted. In the case of a parameter name, only
 * the spectra created from that parameter will be plotted.
 *
 * @param plotType    The plot type specifying what to plot.
 */
void IndirectFitAnalysisTab::plotResult(const QString &plotType) {
  const auto resultWorkspaces = m_fittingModel->getResultWorkspace();
  if (resultWorkspaces) {
    if (plotType.compare("All") == 0)
      plotAll(resultWorkspaces);
    else
      plotParameter(resultWorkspaces, plotType.toStdString());
  }
}

void IndirectFitAnalysisTab::plotAll(
    Mantid::API::WorkspaceGroup_sptr workspaces) {
  for (auto &&workspace : *workspaces)
    plotAll(boost::dynamic_pointer_cast<MatrixWorkspace>(workspace));
}

void IndirectFitAnalysisTab::plotParameter(
    Mantid::API::WorkspaceGroup_sptr workspaces, const std::string &parameter) {
  for (auto &&workspace : *workspaces)
    plotParameter(boost::dynamic_pointer_cast<MatrixWorkspace>(workspace),
                  parameter);
}

void IndirectFitAnalysisTab::plotAll(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  const auto name = QString::fromStdString(workspace->getName());
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
    IndirectTab::plotSpectrum(name, static_cast<int>(i));
}

void IndirectFitAnalysisTab::plotParameter(
    Mantid::API::MatrixWorkspace_sptr workspace,
    const std::string &parameterToPlot) {
  const auto name = QString::fromStdString(workspace->getName());
  const auto labels = IndirectTab::extractAxisLabels(workspace, 1);

  for (const auto &parameter : m_fittingModel->getFitParameterNames()) {
    if (boost::contains(parameter, parameterToPlot)) {
      auto it = labels.find(parameter);
      if (it != labels.end())
        IndirectTab::plotSpectrum(name, static_cast<int>(it->second));
    }
  }
}

/**
 * Executes the single fit algorithm defined in this indirect fit analysis tab.
 */
void IndirectFitAnalysisTab::singleFit() {
  singleFit(getSelectedDataIndex(), getSelectedSpectrum());
}

void IndirectFitAnalysisTab::singleFit(std::size_t dataIndex,
                                       std::size_t spectrum) {
  if (validate())
    runSingleFit(m_fittingModel->getSingleFit(dataIndex, spectrum));
}

/**
 * Executes the sequential fit algorithm defined in this indirect fit analysis
 * tab.
 */
void IndirectFitAnalysisTab::executeFit() {
  if (validate())
    runFitAlgorithm(m_fittingModel->getFittingAlgorithm());
}

bool IndirectFitAnalysisTab::validate() {
  UserInputValidator validator;
  m_dataPresenter->validate(validator);
  m_spectrumPresenter->validate(validator);

  const auto invalidFunction = m_fittingModel->isInvalidFunction();
  if (invalidFunction)
    validator.addErrorMessage(QString::fromStdString(*invalidFunction));

  const auto error = validator.generateErrorMessage();
  emit showMessageBox(error);
  return error.isEmpty();
}

/**
 * Called when the 'Run' button is called in the IndirectTab.
 */
void IndirectFitAnalysisTab::run() {
  runFitAlgorithm(m_fittingModel->getFittingAlgorithm());
}

void IndirectFitAnalysisTab::setAlgorithmProperties(
    IAlgorithm_sptr fitAlgorithm) const {
  fitAlgorithm->setProperty("Minimizer", m_fitPropertyBrowser->minimizer(true));
  fitAlgorithm->setProperty("MaxIterations",
                            m_fitPropertyBrowser->maxIterations());
  fitAlgorithm->setProperty("ConvolveMembers",
                            m_fitPropertyBrowser->convolveMembers());
  fitAlgorithm->setProperty("PeakRadius",
                            m_fitPropertyBrowser->getPeakRadius());
  fitAlgorithm->setProperty("CostFunction",
                            m_fitPropertyBrowser->costFunction());
  fitAlgorithm->setProperty("IgnoreInvalidData",
                            m_fitPropertyBrowser->ignoreInvalidData());

  if (m_fitPropertyBrowser->isHistogramFit())
    fitAlgorithm->setProperty("EvaluationType", "Histogram");
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(updateFitOutput(bool)));
  setupFit(fitAlgorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::runSingleFit(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(updateSingleFitOutput(bool)));
  setupFit(fitAlgorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::setupFit(IAlgorithm_sptr fitAlgorithm) {
  disconnect(m_fitPropertyBrowser,
             SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
             SLOT(updateGuessPlots()));

  setAlgorithmProperties(fitAlgorithm);

  m_fittingAlgorithm = fitAlgorithm;
  m_spectrumPresenter->disableView();
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(fitAlgorithmComplete(bool)));
}

/**
 * Updates the specified combo box, with the available plot options.
 *
 * @param cbPlotType  The combo box.
 */
void IndirectFitAnalysisTab::updatePlotOptions(QComboBox *cbPlotType) {
  setPlotOptions(cbPlotType, m_fittingModel->getFitParameterNames());
}

/**
 * Fills the specified combo box, with the specified parameters.
 *
 * @param cbPlotType  The combo box.
 * @param parameters  The parameters.
 */
void IndirectFitAnalysisTab::setPlotOptions(
    QComboBox *cbPlotType, const std::vector<std::string> &parameters) const {
  cbPlotType->clear();
  QSet<QString> plotOptions;

  for (const auto &parameter : parameters) {
    auto plotOption = QString::fromStdString(parameter);
    auto index = plotOption.lastIndexOf(".");
    if (index >= 0)
      plotOption = plotOption.remove(0, index + 1);
    plotOptions << plotOption;
  }
  setPlotOptions(cbPlotType, plotOptions);
}

/**
 * Fills the specified combo box, with the specified options.
 *
 * @param cbPlotType  The combo box.
 * @param parameters  The options.
 */
void IndirectFitAnalysisTab::setPlotOptions(
    QComboBox *cbPlotType, const QSet<QString> &options) const {
  cbPlotType->clear();

  QStringList plotList;
  if (!options.isEmpty())
    plotList << "All";
  plotList.append(options.toList());
  cbPlotType->addItems(plotList);
}

/**
 * Updates whether the options for plotting and saving fit results are
 * enabled/disabled.
 */
void IndirectFitAnalysisTab::updateResultOptions() {
  const bool isFit = m_fittingModel->isPreviouslyFit(getSelectedDataIndex(),
                                                     getSelectedSpectrum());
  setPlotResultEnabled(isFit);
  setSaveResultEnabled(isFit);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
