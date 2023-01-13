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

#include <map>

namespace {

// A map of the message to display for a returned exception.
static std::map<std::string, std::string> const EXCEPTION_MAP{
    {"X arrays must match when dividing 2D workspaces.",
     "Vanadium normalisation failed:\nX arrays must match when dividing two workspaces."}};

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentPresenter::ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
  m_view->setUpInstrument(m_model->loadedWsName());
}

QWidget *ALFInstrumentPresenter::getSampleLoadWidget() { return m_view->generateSampleLoadWidget(); }

QWidget *ALFInstrumentPresenter::getVanadiumLoadWidget() { return m_view->generateVanadiumLoadWidget(); }

ALFInstrumentWidget *ALFInstrumentPresenter::getInstrumentView() { return m_view->getInstrumentView(); }

void ALFInstrumentPresenter::subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) {
  m_analysisPresenter = presenter;
}

void ALFInstrumentPresenter::loadSettings() { m_view->loadSettings(); }

void ALFInstrumentPresenter::saveSettings() { m_view->saveSettings(); }

void ALFInstrumentPresenter::loadSample() {
  m_analysisPresenter->clear();

  if (auto const filepath = m_view->getSampleFile()) {
    m_model->setSample(loadAndNormalise(*filepath));
    m_view->setSampleRun(std::to_string(m_model->sampleRun()));
  } else {
    m_model->setSample(nullptr);
  }

  generateLoadedWorkspace();
}

void ALFInstrumentPresenter::loadVanadium() {
  m_analysisPresenter->clear();

  if (auto const filepath = m_view->getVanadiumFile()) {
    m_model->setVanadium(loadAndNormalise(*filepath));
    m_view->setVanadiumRun(std::to_string(m_model->vanadiumRun()));
  } else {
    m_model->setVanadium(nullptr);
  }

  generateLoadedWorkspace();
}

Mantid::API::MatrixWorkspace_sptr ALFInstrumentPresenter::loadAndNormalise(const std::string &pathToRun) {
  try {
    return m_model->loadAndNormalise(pathToRun);
  } catch (std::exception const &ex) {
    m_view->warningBox(ex.what());
    return nullptr;
  }
}

void ALFInstrumentPresenter::generateLoadedWorkspace() {
  try {
    m_model->generateLoadedWorkspace();
  } catch (std::exception const &ex) {
    auto const iter = EXCEPTION_MAP.find(ex.what());
    m_view->warningBox(iter != EXCEPTION_MAP.cend() ? iter->second : ex.what());
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
