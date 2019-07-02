// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IPYTHONRUNNER_H
#define MANTID_ISISREFLECTOMETRY_IPYTHONRUNNER_H

#include <string>

#include "GUI/Runs/IRunsPresenter.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class IPythonRunner

IPythonRunner is an interface for running python code
*/
class IPythonRunner {
public:
  virtual ~IPythonRunner(){};
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
