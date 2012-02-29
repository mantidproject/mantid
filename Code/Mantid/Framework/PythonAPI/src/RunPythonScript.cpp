/*WIKI*

C++ algorithm that will run a snippet of python code.
This is meant to be used by [[LoadLiveData]] to perform some processing.

*WIKI*/

#include "MantidPythonAPI/RunPythonScript.h"
#include "MantidKernel/System.h"
#include "MantidPythonAPI/BoostPython_Silent.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace boost::python;

namespace Mantid
{
namespace PythonAPI
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RunPythonScript)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RunPythonScript::RunPythonScript()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RunPythonScript::~RunPythonScript()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RunPythonScript::name() const { return "RunPythonScript";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RunPythonScript::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RunPythonScript::category() const { return "DataHandling\\LiveData";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RunPythonScript::initDocs()
  {
    this->setWikiSummary("Run a short snipped of python code as an algorithm");
    this->setOptionalMessage("Run a short snipped of python code as an algorithm");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RunPythonScript::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input),
        "An input workspace that the python code will modify.\n"
        "The name of the workspace will be in the python variable named 'input'.");

    declareProperty(new PropertyWithValue<std::string>("Code","",Direction::Input),
        "Python code (can be on multiple lines).");

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
        "An output workspace to be produced by the python code.\n"
        "The python code should create the workspace named by the python variable 'output'.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RunPythonScript::exec()
  {
    std::string Code = getPropertyValue("Code");
    std::string inputName = this->getPropertyValue("InputWorkspace");
    std::string outputName = this->getPropertyValue("OutputWorkspace");

    // Initialization of python - run this before anything else
    Py_Initialize();

//    object result1 = eval("12 + 34");
//    double val = extract<double>(result1);
//    std::cout << val << std::endl;

    // Retrieve the main module.
    object main = import("__main__");

    // Retrieve the main module's namespace
    object globals(main.attr("__dict__"));

    // Python variable called 'input' that contains the NAME of the input workspace
    globals["input"] = inputName;
    // Python variable called 'output' that contains the NAME of the output workspace
    globals["output"] = outputName;

    // Handle the output workspace.
    Workspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<Workspace>(outputName);
    this->setProperty("OutputWorkspace", outWS);

    // Execute the Code string
    object result = boost::python::exec(Code.c_str(), globals, globals);

    UNUSED_ARG(result);

    Py_Finalize();

  }



} // namespace Mantid
} // namespace PythonAPI
