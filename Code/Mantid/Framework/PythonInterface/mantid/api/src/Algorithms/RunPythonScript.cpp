/*WIKI*
Algorithm that will run a snippet of python code.
This is meant to be used by [[LoadLiveData]] to perform some processing.

The input & output workspaces can be accessed from the Python code using the variable
names 'input' & 'output' respectively.

*WIKI*/

#include "MantidPythonInterface/api/Algorithms/RunPythonScript.h"
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"
#include "MantidPythonInterface/kernel/Policies/DowncastReturnedValue.h"
#include "MantidKernel/MandatoryValidator.h"

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    /// Algorithm's name for identification. @see Algorithm::name
    const std::string RunPythonScript::name() const { return "RunPythonScript";}

    /// Algorithm's version for identification. @see Algorithm::version
    int RunPythonScript::version() const { return 1;}

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string RunPythonScript::category() const { return "DataHandling\\LiveData\\Support"; }

    /// Sets documentation strings for this algorithm
    void RunPythonScript::initDocs()
    {
      this->setWikiSummary("Executes a snippet of Python code");
      this->setOptionalMessage("Executes a snippet of Python code");
    }

    /** 
     * Override standard group behaviour so that the algorithm is only
     * called once for the whole group
     */
    bool RunPythonScript::checkGroups()
    {
      return false;
    }

    /** 
     * Initialize the algorithm's properties.
     */
    void RunPythonScript::init()
    {
      using namespace API;
      using namespace Kernel;

      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input, PropertyMode::Optional),
          "An input workspace that the python code will modify."
          "The workspace will be in the python variable named 'input'.");
      declareProperty("Code", "", "Python code (can be on multiple lines).", 
                      boost::make_shared<MandatoryValidator<std::string> >());
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output, PropertyMode::Optional),
          "An output workspace to be produced by the python code."
          "The workspace will be in the python variable named 'output'.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void RunPythonScript::exec()
    {
      using namespace API;

      Workspace_sptr outputWS = executeScript(scriptCode());
      setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
    }

    /**
     * Builds the code string from the user input. The user script is wrapped
     * in a PyExec function to 'fool' the Python FrameworkManager into
     * creating a child algorithm for each algorithm that is run. See
     * PythonInterface/mantid/api/src/Exports/FrameworkManager.cpp
     * This has to be the case to get the workspace locking correct.
     *
     * The code assumes that the scope in which it is executed has defined
     * the variables input & output.
     *
     * @return A string containing the code ready to execute
     */
    std::string RunPythonScript::scriptCode() const
    {
      std::string userCode = getPropertyValue("Code");
      // Unify line endings
      boost::regex eol("\\R"); // \R is Perl syntax for matching any EOL sequence
      userCode = boost::regex_replace(userCode, eol, "\n"); // converts all to LF

      // Wrap and indent the user code (see method documentation)
      std::istringstream is(userCode);
      std::ostringstream os;
      os << "from mantid.simpleapi import *\n"
         << "def PyExec(input=None,output=None):\n";
      std::string line;
      while(getline(is, line))
      {
        os << "  " << line << "\n";
      }
      os << "  return input,output\n"; // When executed the global scope needs to know about input,output so we return them
      os << "input,output = PyExec(input,output)";
      return os.str();
    }

    /**
     * Sets up the code context & executes it.
     * A python dictionary of local attributes is setup to contain a reference to the input workspace
     * & the output workspace. This together with the __main__ global dictionary defines the execution
     * context
     * @param script A string containing a read-to-execute script
     * @return A pointer to the output workspace if one was generated. If one was not then this is an empty pointer
     */
    boost::shared_ptr<API::Workspace> RunPythonScript::executeScript(const std::string & script) const
    {
      using namespace API;
      using namespace boost::python;

      auto locals = doExecuteScript(script);
      return extractOutputWorkspace(locals);
    }

    /**
     * Uses the __main__ object to define the globals context and together with the given locals
     * dictionary executes the script. The GIL is acquired and released during this call
     * @param script The script code
     * @returns A dictionary defining the input & output variables
     */
    boost::python::dict RunPythonScript::doExecuteScript(const std::string & script) const
    {
      // Execution
      Environment::GlobalInterpreterLock gil;
      // Retrieve the main module.
      auto main = boost::python::import("__main__");
      // Retrieve the main module's namespace
      boost::python::object globals(main.attr("__dict__"));
      boost::python::dict locals = buildLocals();
      try
      {
        boost::python::exec(script.c_str(), globals, locals);
      }
      catch(boost::python::error_already_set &)
      {
        Environment::throwRuntimeError();
      }
      return locals;
    }

    /**
     * Creates a Python dictionary containing definitions of the 'input' & 'output' variable
     * references that the script may use
     * @return A Python dictionary that can be used as the locals argument for the script execution
     */
    boost::python::dict RunPythonScript::buildLocals() const
    {
      // Define the local variable names required by the script, in this case
      // - input: Points to input workspace if one has been given
      // - output: Will point to the output workspace if one has been given
      using namespace boost::python;

      dict locals;
      locals["input"] = object(); // default to None
      locals["output"] = object();

      API::Workspace_sptr inputWS = getProperty("InputWorkspace");
      if(inputWS)
      {
        // We have a generic workspace ptr but the Python needs to see the derived type so
        // that it can access the appropriate methods for that instance
        // The DowncastReturnedValue policy is already in place for this and is used in many
        // method exports as part of a return_value_policy struct.
        // It is called manually here.
        typedef Policies::DowncastReturnedValue::apply<API::Workspace_sptr>::type WorkspaceDowncaster;
        locals["input"] = object(handle<>(WorkspaceDowncaster()(inputWS)));
      }
      std::string outputWSName = getPropertyValue("OutputWorkspace");
      if(!outputWSName.empty())
      {
        locals["output"] = object(handle<>(to_python_value<const std::string&>()(outputWSName)));
      }
      return locals;
    }


    /**
     * If an output workspace was created then extract it from the given dictionary
     * @param locals A dictionary possibly containing an 'output' reference
     * @return A pointer to the output workspace if created, otherwise an empty pointer
     */
    boost::shared_ptr<API::Workspace> RunPythonScript::extractOutputWorkspace(const boost::python::dict & locals) const
    {
      using namespace API;
      using namespace boost::python;

      // Might be None, string or a workspace object
      object pyoutput = locals["output"];
      if(pyoutput.ptr() == Py_None) return Workspace_sptr();

      extract<Workspace_sptr> workspaceExtractor(pyoutput);
      if(workspaceExtractor.check())
      {
        return workspaceExtractor();
      }
      else
      {
        extract<std::string> stringExtractor(pyoutput);
        if(stringExtractor.check())
        {
          // Will raise an error if the workspace does not exist as the user requested an output workspace
          // but didn't create one.
          return AnalysisDataService::Instance().retrieve(stringExtractor());
        }
        else
        {
          throw std::runtime_error("Invalid type assigned to 'output' variable. Must be a string or a Workspace object");
        }
      }
    }

  } // namespace PythonInterface
} // namespace Mantid
