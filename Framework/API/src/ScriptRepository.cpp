// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ScriptRepository.h"

namespace Mantid::API {

ScriptRepoException::ScriptRepoException(const std::string &info, const std::string &system, const std::string &file,
                                         int line) {
  _system_error = system;
  _user_info = info;

  if (file.empty()) {
    _file_path = "Not provided";
  } else {
    _file_path = file;
    if (line > 0) {
      _file_path.append(": ").append(std::to_string(line));
    }
  }
}

const char *ScriptRepoException::what() const noexcept { return _user_info.c_str(); }

} // namespace Mantid::API
