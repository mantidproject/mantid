// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IPyRunner

IPyRunner is an interface for running python code
*/
class IPyRunner {
public:
  virtual ~IPyRunner(){};
  virtual void runPythonCode(std::string const &pythonCode) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
