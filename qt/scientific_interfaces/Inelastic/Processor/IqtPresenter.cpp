// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtPresenter.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <tuple>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("Iqt");
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IqtPresenter::IqtPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner,
                           IIqtView *view, std::unique_ptr<IIqtModel> model)
    : DataProcessor(parent, std::move(algorithmRunner)), m_view(view), m_model(std::move(model)),
      m_selectedSpectrum(0) {
  m_view->subscribePresenter(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::SpectraTiled));
  m_view->setup();
}

void IqtPresenter::handleSampDataReady(const std::string &wsname) {
  try {
    auto workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);
    setInputWorkspace(std::move(workspace));
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    m_view->showMessageBox("Unable to retrieve workspace: " + wsname);
    m_view->setPreviewSpectrumMaximum(0);
    return;
  }
  m_view->setPreviewSpectrumMaximum(static_cast<int>(getInputWorkspace()->getNumberHistograms()) - 1);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  m_view->setRangeSelectorDefault(getInputWorkspace(), WorkspaceUtils::getXRangeFromWorkspace(getInputWorkspace()));
  m_view->updateDisplayedBinParameters();
}

void IqtPresenter::handleResDataReady(const std::string &resWorkspace) {
  m_view->updateDisplayedBinParameters();
  m_model->setResWorkspace(resWorkspace);
}

void IqtPresenter::handleIterationsChanged(int iterations) { m_model->setNIterations(std::to_string(iterations)); }

void IqtPresenter::handleRun() {
  clearOutputPlotOptionsWorkspaces();
  m_view->setWatchADS(false);
  m_view->setSaveResultEnabled(false);

  m_view->updateDisplayedBinParameters();

  // Construct the result workspace for Python script export
  std::string sampleName = m_view->getSampleName();
  m_pythonExportWsName = sampleName.replace(sampleName.find_last_of("_"), sampleName.size(), "_iqt");
  m_algorithmRunner->execute(m_model->setupTransformToIqt(m_pythonExportWsName));
}
/**
 * Handle saving of workspace
 */
void IqtPresenter::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    m_algorithmRunner->execute(setupSaveAlgorithm(m_pythonExportWsName));
  ;
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void IqtPresenter::handlePlotCurrentPreview() {
  auto const &inputWs = getInputWorkspace();
  auto const &index = boost::numeric_cast<size_t>(m_selectedSpectrum);

  if (inputWs && index < inputWs->getNumberHistograms())
    m_plotter->plotSpectra(inputWs->getName(), std::to_string(index), SettingsHelper::externalPlotErrorBars());
  else
    m_view->showMessageBox("Workspace not found - data may not be loaded.");
}

void IqtPresenter::handleErrorsClicked(int state) { m_model->setCalculateErrors(state); }

void IqtPresenter::handleNormalizationClicked(int state) { m_model->setEnforceNormalization(state); }

void IqtPresenter::handleValueChanged(std::string const &propName, double value) {
  if (propName == "ELow") {
    m_model->setEnergyMin(value);
  } else if (propName == "EHigh") {
    m_model->setEnergyMax(value);
  } else if (propName == "SampleBinning") {
    m_model->setNumBins(value);
  }
}

void IqtPresenter::handlePreviewSpectrumChanged(int spectra) {
  setSelectedSpectrum(spectra);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

/**
 * Handle algorithm completion.
 *
 * @param error If the algorithm failed
 */
void IqtPresenter::runComplete(Mantid::API::IAlgorithm_sptr const algorithm, bool const error) {
  (void)algorithm;
  m_view->setWatchADS(true);
  m_view->setSaveResultEnabled(!error);
  if (!error)
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
void IqtPresenter::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_view->getDataSelector("sample"));
  validator->checkDataSelectorIsValid("Resolution", m_view->getDataSelector("resolution"));
  if (m_model->EMin() >= m_model->EMax())
    validator->addErrorMessage("ELow must be less than EHigh.\n");
}

void IqtPresenter::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Iqt");
  m_view->setSampleFBSuffixes(filter ? InterfaceUtils::getSampleFBSuffixes(tabName)
                                     : InterfaceUtils::getExtensions(tabName));
  m_view->setSampleWSSuffixes(filter ? InterfaceUtils::getSampleWSSuffixes(tabName) : noSuffixes);
  m_view->setResolutionFBSuffixes(filter ? InterfaceUtils::getResolutionFBSuffixes(tabName)
                                         : InterfaceUtils::getExtensions(tabName));
  m_view->setResolutionWSSuffixes(filter ? InterfaceUtils::getResolutionWSSuffixes(tabName) : noSuffixes);
}

void IqtPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int IqtPresenter::getSelectedSpectrum() const { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void IqtPresenter::setSelectedSpectrum(int spectrum) { m_selectedSpectrum = spectrum; }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr IqtPresenter::getInputWorkspace() const { return m_inputWorkspace; }

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void IqtPresenter::setInputWorkspace(MatrixWorkspace_sptr inputWorkspace) {
  m_model->setSampleWorkspace(inputWorkspace->getName());
  m_inputWorkspace = std::move(inputWorkspace);
}

} // namespace CustomInterfaces
} // namespace MantidQt
