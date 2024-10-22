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

int OutputNamePresenter::findInsertIndexLabel(std::string const &outputBasename) {
  std::vector<int> suffixPos;
  std::transform(m_suffixes.begin(), m_suffixes.end(), std::back_inserter(suffixPos),
                 [&](auto const &suffix) { return outputBasename.rfind(suffix); });
  auto max_index = (*std::max_element(suffixPos.cbegin(), suffixPos.cend()));
  return (suffixPos.empty() || (max_index == -1)) ? static_cast<int>(outputBasename.length()) : max_index;
}

void OutputNamePresenter::generateLabelWarning() const {
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
  return outputName.insert(findInsertIndexLabel(outputName), addLabelUnderscore(m_view->getCurrentLabel()));
}

void OutputNamePresenter::handleUpdateOutputLabel() {
  auto labelName = m_currBasename;
  if (!m_view->getCurrentLabel().empty())
    labelName.insert(findInsertIndexLabel(labelName), "_" + m_view->getCurrentLabel());
  labelName += m_currOutputSuffix;
  m_view->setOutputNameLabel(labelName);
  generateLabelWarning();
}

} // namespace MantidQt::CustomInterfaces
