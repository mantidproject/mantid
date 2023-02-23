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
    : m_view(view), m_model(std::move(model)), m_algorithmManager(std::move(algorithmManager)) {
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

void ALFInstrumentPresenter::notifyAlgorithmError(std::string const &message) { m_view->warningBox(message); }

void ALFInstrumentPresenter::loadSample() {
  m_analysisPresenter->clear();
  if (auto const filepath = m_view->getSampleFile()) {
    m_algorithmManager->loadAndNormalise(ALFDataType::SAMPLE, *filepath);
  } else {
    m_model->setSample(nullptr);
    generateLoadedWorkspace();
  }
}

void ALFInstrumentPresenter::loadVanadium() {
  m_analysisPresenter->clear();

  if (auto const filepath = m_view->getVanadiumFile()) {
    m_algorithmManager->loadAndNormalise(ALFDataType::VANADIUM, *filepath);
  } else {
    m_model->setVanadium(nullptr);
    generateLoadedWorkspace();
  }
}

void ALFInstrumentPresenter::notifyLoadAndNormaliseComplete(ALFDataType const &dataType,
                                                            Mantid::API::MatrixWorkspace_sptr const &workspace) {
  switch (dataType) {
  case ALFDataType::SAMPLE:
    m_model->setSample(workspace);
    m_view->setSampleRun(std::to_string(m_model->sampleRun()));
    break;
  case ALFDataType::VANADIUM:
    m_model->setVanadium(workspace);
    m_view->setVanadiumRun(std::to_string(m_model->vanadiumRun()));
    break;
  }
  generateLoadedWorkspace();
}

void ALFInstrumentPresenter::generateLoadedWorkspace() {
  try {
    m_model->generateLoadedWorkspace();
  } catch (std::exception const &ex) {
    m_view->warningBox(std::string("Vanadium normalisation failed: ") + ex.what());
  }
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

void ALFInstrumentPresenter::updateInstrumentViewFromModel() {
  m_view->clearShapes();
  m_view->drawRectanglesAbove(m_model->selectedTubes());
}

void ALFInstrumentPresenter::updateAnalysisViewFromModel() {
  auto const [workspace, twoThetas] = m_model->generateOutOfPlaneAngleWorkspace(m_view->getInstrumentActor());
  m_analysisPresenter->setExtractedWorkspace(workspace, twoThetas);
}

} // namespace MantidQt::CustomInterfaces
