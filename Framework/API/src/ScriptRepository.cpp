// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ScriptRepository.h"

namespace Mantid::API {

ScriptRepoException::ScriptRepoException(const std::string &info, const std::string &system, const std::string &file,
                                         int line)
    : m_systemError(system), m_userInfo(info) {

  if (file.empty()) {
    m_filepath = "Not provided";
  } else {
    m_filepath = file;
    if (line > 0) {
      m_filepath.append(": ").append(std::to_string(line));
    }
  }
}

const char *ScriptRepoException::what() const noexcept { return m_userInfo.c_str(); }

} // namespace Mantid::API
