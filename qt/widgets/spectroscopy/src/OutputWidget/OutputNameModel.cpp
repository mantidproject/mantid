// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameModel.h"

namespace MantidQt::CustomInterfaces {

OutputNameModel::OutputNameModel() : m_suffixes(), m_currBasename(), m_currOutputSuffix() {}

void OutputNameModel::setSuffixes(std::vector<std::string> const &suffixes) { m_suffixes = suffixes; }

std::vector<std::string> OutputNameModel::suffixes() const { return m_suffixes; }

void OutputNameModel::setOutputBasename(std::string const &outputBasename) { m_currBasename = outputBasename; }

std::string OutputNameModel::outputBasename() const { return m_currBasename; }

void OutputNameModel::setOutputSuffix(std::string const &outputSuffix) { m_currOutputSuffix = outputSuffix; }

std::string OutputNameModel::outputSuffix() const { return m_currOutputSuffix; }

int OutputNameModel::findIndexToInsertLabel(std::string const &basename) {
  int maxPos = -1;
  for (auto const &suffix : m_suffixes) {
    auto pos = static_cast<int>(basename.rfind(suffix));
    maxPos = pos >= maxPos ? pos : maxPos;
  }
  return (m_suffixes.empty() || (maxPos == -1)) ? static_cast<int>(basename.length()) : maxPos;
}
} // namespace MantidQt::CustomInterfaces
