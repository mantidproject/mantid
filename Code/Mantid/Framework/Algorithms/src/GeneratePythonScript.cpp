#include "MantidAlgorithms/GeneratePythonScript.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/ScriptBuilder.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GeneratePythonScript)

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void GeneratePythonScript::init()
{
  declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input), "An input workspace.");

  std::vector<std::string> exts;
  exts.push_back(".py");

  declareProperty(new API::FileProperty("Filename","", API::FileProperty::OptionalSave, exts),
  "The name of the file into which the workspace history will be generated.");
  declareProperty("ScriptText", std::string(""), "Saves the history of the workspace to a variable.", Direction::Output);

  declareProperty("UnrollAll", false, "Unroll all algorithms to show just there child algorithms.", Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void GeneratePythonScript::exec()
{
  const Workspace_const_sptr ws = getProperty("InputWorkspace");
  const bool unrollAll = getProperty("UnrollAll");

  // Get the algorithm histories of the workspace.
  const WorkspaceHistory wsHistory = ws->getHistory();

  auto view = wsHistory.createView();

  if(unrollAll)
  {
    view->unrollAll();
  }

  ScriptBuilder builder(view);
  std::string generatedScript = builder.build();

  setPropertyValue("ScriptText", generatedScript);

  const std::string filename = getPropertyValue("Filename");

  if (!filename.empty())
  {
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    file << generatedScript;
    file.flush();
    file.close();
  }
}

} // namespace Algorithms
} // namespace Mantid
