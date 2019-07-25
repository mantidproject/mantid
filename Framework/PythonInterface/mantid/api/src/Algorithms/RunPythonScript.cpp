// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/Algorithms/RunPythonScript.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/ExtractWorkspace.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/IsNone.h"

#include <boost/python/call_method.hpp>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/to_python_value.hpp>
#include <boost/regex.hpp>
#include <fstream>

namespace Mantid {
namespace PythonInterface {

using namespace API;
using namespace Kernel;

/// Algorithm's name for identification. @see Algorithm::name
const std::string RunPythonScript::name() const { return "RunPythonScript"; }

/// Algorithm's version for identification. @see Algorithm::version
int RunPythonScript::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RunPythonScript::category() const {
  return "DataHandling\\LiveData\\Support";
}

/// @copydoc Algorithm::summary
const std::string RunPythonScript::summary() const {
  return "Executes a snippet of Python code";
}

/**
 * Override standard group behaviour so that the algorithm is only
 * called once for the whole group
 */
bool RunPythonScript::checkGroups() { return false; }

/**
 * Initialize the algorithm's properties.
 */
void RunPythonScript::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Optional),
      "An input workspace that the python code will modify."
      "The workspace will be in the python variable named 'input'.");
  declareProperty("Code", "", "Python code (can be on multiple lines).");
  declareProperty(std::make_unique<FileProperty>(
                      "Filename", "", FileProperty::OptionalLoad, "py"),
                  "A File containing a python script");
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>(
          "OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
      "An output workspace to be produced by the python code."
      "The workspace will be in the python variable named 'output'.");
}

std::map<std::string, std::string> RunPythonScript::validateInputs() {
  std::map<std::string, std::string> out;

  bool hasCode = (!this->getPropertyValue("Code").empty());
  bool hasFile = (!this->getPropertyValue("Filename").empty());

  std::string msg;
  if ((!hasCode) && (!hasFile)) {
    msg = "Must specify python to execute";
  }

  if (!msg.empty()) {
    out["Code"] = msg;
    out["Filename"] = msg;
  }

  return out;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RunPythonScript::exec() {
  Workspace_sptr outputWS = executeScript(scriptCode());
  setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
}

/**
 * Builds the code string from the user input. The user script is wrapped
 * in a tiny PythonAlgorithm to 'fool' the Python framework into
 * creating a child algorithm for each algorithm that is run. See
 * PythonInterface/mantid/simpleapi.py:_create_algorithm_object
 * This has to be the case to get the workspace locking correct.
 *
 * The code assumes that the scope in which it is executed has defined
 * the variables input & output.
 *
 * @return A string containing the code ready to execute
 */
std::string RunPythonScript::scriptCode() const {
  std::string userCode = getPropertyValue("Code");
  std::string filename = getPropertyValue("Filename");

  // fill userCode with the contents of the supplied file if it wasn't already
  // supplied
  if (userCode.empty() && (!filename.empty())) {
    std::ifstream handle(filename.c_str(), std::ios_base::in);
    if (!handle.is_open()) {
      // throw exception if file cannot be opened
      std::stringstream errss;
      errss << "Unable to open file " << filename;
      throw std::runtime_error(errss.str());
    }

    std::stringstream buffer;
    buffer << handle.rdbuf();
    userCode = buffer.str();
  }

  if (userCode.empty()) {
    throw std::runtime_error("Python script is empty");
  }

  // Unify line endings
  boost::regex eol("\\R"); // \R is Perl syntax for matching any EOL sequence
  userCode = boost::regex_replace(userCode, eol, "\n"); // converts all to LF

  // Wrap and indent the user code (see method documentation)
  std::istringstream is(userCode);
  std::ostringstream os;
  const char *indent = "    ";
  os << "import mantid\n"
     << "from mantid.simpleapi import *\n"
     << "class _DUMMY_ALG(mantid.api.PythonAlgorithm):\n"
     << indent << "def PyExec(self, input=None,output=None):\n";
  std::string line;
  while (getline(is, line)) {
    os << indent << indent << line << "\n";
  }
  os << indent << indent
     << "return input,output\n"; // When executed the global scope needs to know
                                 // about input,output so we return them
  os << "input,output = _DUMMY_ALG().PyExec(input,output)";

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG))
    g_log.debug() << "Full code to be executed:\n" << os.str() << "\n";
  return os.str();
}

/**
 * Sets up the code context & executes it.
 * A python dictionary of local attributes is setup to contain a reference to
 * the input workspace
 * & the output workspace. This together with the __main__ global dictionary
 * defines the execution
 * context
 * @param script A string containing a read-to-execute script
 * @return A pointer to the output workspace if one was generated. If one was
 * not then this is an empty pointer
 */
boost::shared_ptr<API::Workspace>
RunPythonScript::executeScript(const std::string &script) const {
  using namespace API;
  using namespace boost::python;

  // Execution
  GlobalInterpreterLock gil;
  auto locals = doExecuteScript(script);
  return extractOutputWorkspace(locals);
}

/**
 * Uses the __main__ object to define the globals context and together with the
 * given locals
 * dictionary executes the script. The GIL is acquired and released during this
 * call
 * @param script The script code
 * @returns A dictionary defining the input & output variables
 */
boost::python::dict
RunPythonScript::doExecuteScript(const std::string &script) const {
  // Retrieve the main module.
  auto main = boost::python::import("__main__");
  // Retrieve the main module's namespace
  boost::python::object globals(main.attr("__dict__"));
  boost::python::dict locals = buildLocals();
  try {
    boost::python::exec(script.c_str(), globals, locals);
  } catch (boost::python::error_already_set &) {
    throw PythonException();
  }
  return locals;
}

/**
 * Creates a Python dictionary containing definitions of the 'input' & 'output'
 * variable
 * references that the script may use
 * @return A Python dictionary that can be used as the locals argument for the
 * script execution
 */
boost::python::dict RunPythonScript::buildLocals() const {
  // Define the local variable names required by the script, in this case
  // - input: Points to input workspace if one has been given
  // - output: Will point to the output workspace if one has been given
  using namespace boost::python;

  dict locals;
  locals["input"] = object(); // default to None
  locals["output"] = object();

  API::Workspace_sptr inputWS = getProperty("InputWorkspace");
  if (inputWS) {
    locals["input"] =
        object(handle<>(to_python_value<API::Workspace_sptr>()(inputWS)));
  }
  std::string outputWSName = getPropertyValue("OutputWorkspace");
  if (!outputWSName.empty()) {
    locals["output"] =
        object(handle<>(to_python_value<const std::string &>()(outputWSName)));
  }
  return locals;
}

/**
 * If an output workspace was created then extract it from the given dictionary
 * @param locals A dictionary possibly containing an 'output' reference
 * @return A pointer to the output workspace if created, otherwise an empty
 * pointer
 */
boost::shared_ptr<API::Workspace> RunPythonScript::extractOutputWorkspace(
    const boost::python::dict &locals) const {
  using namespace API;
  using namespace boost::python;

  // Might be None, string or a workspace object
  auto pyoutput = locals.get("output");
  if (isNone(pyoutput))
    return Workspace_sptr();

  auto ptrExtract = ExtractWorkspace(pyoutput);
  if (ptrExtract.check()) {
    return ptrExtract();
  } else {
    extract<std::string> extractString(pyoutput);
    if (extractString.check()) {
      // Will raise an error if the workspace does not exist as the user
      // requested
      // an output workspace
      // but didn't create one.
      return AnalysisDataService::Instance().retrieve(extractString());
    } else {
      throw std::runtime_error(
          "Invalid type assigned to 'output' variable. Must "
          "be a string or a Workspace object");
    }
  }
}

} // namespace PythonInterface
} // namespace Mantid
