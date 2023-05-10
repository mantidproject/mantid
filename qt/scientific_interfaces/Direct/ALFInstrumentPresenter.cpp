// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentPresenter.h"

#include "ALFAnalysisPresenter.h"
#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt::CustomInterfaces {

ALFInstrumentPresenter::ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model,
                                               std::unique_ptr<IALFAlgorithmManager> algorithmManager)
    : m_dataSwitch(), m_view(view), m_model(std::move(model)), m_algorithmManager(std::move(algorithmManager)) {
  m_view->subscribePresenter(this);
  m_view->setUpInstrument(m_model->loadedWsName());
  m_algorithmManager->subscribe(this);
}

QWidget *ALFInstrumentPresenter::getSampleLoadWidget() { return m_view->generateSampleLoadWidget(); }

QWidget *ALFInstrumentPresenter::getVanadiumLoadWidget() { return m_view->generateVanadiumLoadWidget(); }

ALFInstrumentWidget *ALFInstrumentPresenter::getInstrumentView() { return m_view->getInstrumentView(); }

void ALFInstrumentPresenter::subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) {
  m_analysisPresenter = presenter;
}

void ALFInstrumentPresenter::loadSettings() { m_view->loadSettings(); }

void ALFInstrumentPresenter::saveSettings() { m_view->saveSettings(); }

void ALFInstrumentPresenter::notifyAlgorithmError(std::string const &message) {
  m_view->enable();
  m_view->displayWarning(message);
}

void ALFInstrumentPresenter::loadSample() {
  m_dataSwitch = ALFData::SAMPLE;
  loadAndNormalise();
}

void ALFInstrumentPresenter::loadVanadium() {
  m_dataSwitch = ALFData::VANADIUM;
  loadAndNormalise();
}

void ALFInstrumentPresenter::loadAndNormalise() {
  m_view->disable(m_dataSwitch == ALFData::SAMPLE ? "Loading sample" : "Loading vanadium");
  m_analysisPresenter->clear();

  if (auto const filepath = getFileFromView()) {
    m_algorithmManager->load(m_model->loadProperties(*filepath));
  } else {
    m_model->setData(m_dataSwitch, nullptr);
    generateLoadedWorkspace();
  }
}

void ALFInstrumentPresenter::notifyLoadComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  if (m_model->isALFData(workspace)) {
    m_algorithmManager->normaliseByCurrent(m_model->normaliseByCurrentProperties(workspace));
  } else {
    m_view->enable();
    m_view->displayWarning("The loaded data is not from the ALF instrument");
  }
}

void ALFInstrumentPresenter::notifyNormaliseByCurrentComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_model->setData(m_dataSwitch, workspace);
  updateRunInViewFromModel();
  generateLoadedWorkspace();
}

void ALFInstrumentPresenter::generateLoadedWorkspace() {
  if (!m_model->hasData(ALFData::SAMPLE)) {
    m_view->enable();
    return;
  }

  // Rebin the vanadium to match the sample binning if the bins do not match
  if (m_model->binningMismatch()) {
    m_algorithmManager->rebinToWorkspace(m_model->rebinToWorkspaceProperties());
  } else {
    normaliseSampleByVanadium();
  }
}

void ALFInstrumentPresenter::notifyRebinToWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_model->setData(ALFData::VANADIUM, workspace);
  normaliseSampleByVanadium();
}

void ALFInstrumentPresenter::normaliseSampleByVanadium() {
  // Normalise the sample by the vanadium and replace any special values if a vanadium exists
  if (m_model->hasData(ALFData::VANADIUM)) {
    m_algorithmManager->divide(m_model->divideProperties());
  } else {
    convertSampleToDSpacing(m_model->data(ALFData::SAMPLE));
  }
}

void ALFInstrumentPresenter::notifyDivideComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_algorithmManager->replaceSpecialValues(m_model->replaceSpecialValuesProperties(workspace));
}

void ALFInstrumentPresenter::notifyReplaceSpecialValuesComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  convertSampleToDSpacing(workspace);
}

void ALFInstrumentPresenter::convertSampleToDSpacing(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  // Convert the sample to DSpacing if not already in dspacing
  if (!m_model->axisIsDSpacing()) {
    m_algorithmManager->convertUnits(m_model->convertUnitsProperties(workspace));
  } else {
    notifyConvertUnitsComplete(workspace);
  }
}

void ALFInstrumentPresenter::notifyConvertUnitsComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_model->replaceSampleWorkspaceInADS(workspace);
  m_view->enable();
}

void ALFInstrumentPresenter::notifyInstrumentActorReset() { updateAnalysisViewFromModel(); }

void ALFInstrumentPresenter::notifyShapeChanged() {
  if (m_model->setSelectedTubes(m_view->getSelectedDetectors())) {
    updateInstrumentViewFromModel();
    updateAnalysisViewFromModel();
  }
}

void ALFInstrumentPresenter::notifyTubesSelected(std::vector<DetectorTube> const &tubes) {
  if (!tubes.empty() && m_model->addSelectedTube(tubes.front())) {
    updateInstrumentViewFromModel();
    updateAnalysisViewFromModel();
  }
}

std::optional<std::string> ALFInstrumentPresenter::getFileFromView() const {
  switch (m_dataSwitch) {
  case ALFData::SAMPLE:
    return m_view->getSampleFile();
  case ALFData::VANADIUM:
    return m_view->getVanadiumFile();
  }
  throw std::invalid_argument("ALFData must be one of { SAMPLE, VANADIUM }");
}

void ALFInstrumentPresenter::updateRunInViewFromModel() {
  auto const runAsString = std::to_string(m_model->run(m_dataSwitch));
  switch (m_dataSwitch) {
  case ALFData::SAMPLE:
    m_view->setSampleRun(runAsString);
    return;
  case ALFData::VANADIUM:
    m_view->setVanadiumRun(runAsString);
    return;
  }
  throw std::invalid_argument("ALFData must be one of { SAMPLE, VANADIUM }");
}

void ALFInstrumentPresenter::updateInstrumentViewFromModel() {
  m_view->clearShapes();
  m_view->drawRectanglesAbove(m_model->selectedTubes());
}

void ALFInstrumentPresenter::updateAnalysisViewFromModel() {
  m_view->disable("Processing selection");
  if (m_model->hasSelectedTubes()) {
    m_algorithmManager->createWorkspace(m_model->createWorkspaceAlgorithmProperties(m_view->getInstrumentActor()));
  } else {
    m_analysisPresenter->setExtractedWorkspace(nullptr, {});
    m_view->enable();
  }
}

void ALFInstrumentPresenter::notifyCreateWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_algorithmManager->scaleX(m_model->scaleXProperties(workspace));
}

void ALFInstrumentPresenter::notifyScaleXComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_algorithmManager->rebunch(m_model->rebunchProperties(workspace));
}

void ALFInstrumentPresenter::notifyRebunchComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_analysisPresenter->setExtractedWorkspace(workspace, m_model->twoThetasClosestToZero());
  m_view->enable();
}

} // namespace MantidQt::CustomInterfaces
