// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

#include "GUI/Runs/IRunsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** @class IPythonRunner

IPythonRunner is an interface for running python code
*/
class IPythonRunner {
public:
  virtual ~IPythonRunner() {};
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
