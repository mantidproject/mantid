// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNamePresenter.h"

#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include <string>

namespace {
std::string addLabelUnderscore(std::string const &label) { return label.empty() ? "" : "_" + label; }
} // namespace

namespace MantidQt::CustomInterfaces {
OutputNamePresenter::OutputNamePresenter(IOutputNameView *view) : m_view(view), m_suffixes(), m_currBasename() {
  m_view->subscribePresenter(this);
}

void OutputNamePresenter::setWsSuffixes(std::vector<std::string> const &suffixes) { m_suffixes = suffixes; }

void OutputNamePresenter::setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix) {
  m_view->enableLabelEditor();
  m_currBasename = outputBasename;
  m_currOutputSuffix = outputSuffix;
  handleUpdateOutputLabel();
}

int OutputNamePresenter::findIndexToInsertLabel(std::string const &outputBasename) {
  int maxPos = -1;
  for (auto const &suffix : m_suffixes) {
    int pos = outputBasename.rfind(suffix);
    maxPos = pos >= maxPos ? pos : maxPos;
  }
  return (m_suffixes.empty() || (maxPos == -1)) ? static_cast<int>(outputBasename.length()) : maxPos;
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
  auto outputName = m_currBasename;
  return outputName.insert(findIndexToInsertLabel(outputName), addLabelUnderscore(m_view->getCurrentLabel()));
}

void OutputNamePresenter::handleUpdateOutputLabel() {
  auto labelName = m_currBasename;
  if (!m_view->getCurrentLabel().empty())
    labelName.insert(findIndexToInsertLabel(labelName), "_" + m_view->getCurrentLabel());
  labelName += m_currOutputSuffix;
  m_view->setOutputNameLabel(labelName);
  generateWarningLabel();
}

} // namespace MantidQt::CustomInterfaces
