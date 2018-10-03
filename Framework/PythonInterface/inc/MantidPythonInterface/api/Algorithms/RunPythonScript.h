// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_
#define MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_

#include "MantidAPI/Algorithm.h"

#include <boost/python/dict.hpp>

namespace Mantid {
namespace PythonInterface {

class DLLExport RunPythonScript : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  bool checkGroups() override;
  void init() override;
  void exec() override;

  /// Return the code string to execute
  std::string scriptCode() const;
  /// Sets up the code context & executes it
  boost::shared_ptr<API::Workspace>
  executeScript(const std::string &script) const;
  /// Execute the code in the given local context
  boost::python::dict doExecuteScript(const std::string &script) const;
  /// Builds the local dictionary that defines part of the execution context of
  /// the script
  boost::python::dict buildLocals() const;
  /// Extracts any output workspace pointer that was created
  boost::shared_ptr<API::Workspace>
  extractOutputWorkspace(const boost::python::dict &locals) const;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_ */
