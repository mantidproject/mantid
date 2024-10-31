// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNamePresenter.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

namespace {
std::string addLabelUnderscore(std::string const &label) { return label.empty() ? "" : "_" + label; }
} // namespace

namespace MantidQt::CustomInterfaces {
OutputNamePresenter::OutputNamePresenter(std::unique_ptr<IOutputNameModel> model, IOutputNameView *view)
    : m_model(std::move(model)), m_view(view) {
  m_view->subscribePresenter(this);
}

void OutputNamePresenter::setWsSuffixes(std::vector<std::string> const &suffixes) { m_model->setSuffixes(suffixes); }

void OutputNamePresenter::setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix) {
  m_view->enableLabelEditor();
  m_model->setOutputBasename(outputBasename);
  m_model->setOutputSuffix(outputSuffix);
  handleUpdateOutputLabel();
}

void OutputNamePresenter::generateWarningLabel() const {
  auto textColor = "color: darkGreen";
  auto text = "Unused name, new workspace will be created";
  if (MantidWidgets::WorkspaceUtils::doesExistInADS(m_view->getCurrentOutputName())) {
    text = "Output Name is in use, workspace will be overriden.";
    textColor = "color: darkRed";
  }
  m_view->setWarningLabel(text, textColor);
}

std::string OutputNamePresenter::generateOutputLabel() {
  auto outputName = m_model->outputBasename();
  return outputName.insert(m_model->findIndexToInsertLabel(outputName), addLabelUnderscore(m_view->getCurrentLabel()));
}

void OutputNamePresenter::handleUpdateOutputLabel() {
  auto labelName = m_model->outputBasename();
  if (!m_view->getCurrentLabel().empty())
    labelName.insert(m_model->findIndexToInsertLabel(labelName), "_" + m_view->getCurrentLabel());
  labelName += m_model->outputSuffix();
  m_view->setOutputNameLabel(labelName);
  generateWarningLabel();
}
} // namespace MantidQt::CustomInterfaces
