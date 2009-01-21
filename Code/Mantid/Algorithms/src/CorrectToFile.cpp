//-----------------------------
// Includes
//----------------------------
#include "MantidAlgorithms/CorrectToFile.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorrectToFile)

// Get a reference to the logger. It is used to print out information, warning and error messages
Mantid::Kernel::Logger& CorrectToFile::g_log = Mantid::Kernel::Logger::get("CorrectToFile");

void CorrectToFile::init()
{
  declareProperty(new API::WorkspaceProperty<>("WorkspaceToCorrect","",Kernel::Direction::Input));
  declareProperty("Filename","",new Kernel::FileValidator());
  std::vector<std::string> operations(1, std::string("Divide"));
  operations.push_back("Multiply");
  declareProperty("WorkspaceOperation", "Divide", new Kernel::ListValidator(operations));
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Kernel::Direction::Output));
}

void CorrectToFile::exec()
{
  //First load in rkh file for correction
  Algorithm_sptr loadRKH = createSubAlgorithm("LoadRKH");
  std::string rkhfile = getProperty("Filename");
  loadRKH->setPropertyValue("Filename", rkhfile);
  loadRKH->setPropertyValue("OutputWorkspace", "rkhout");

  try
  {
    loadRKH->execute();
  }
  catch(std::runtime_error & err)
  {
    g_log.error() << "Unable to successfully run the LoadRKH sub algorithm.";
    throw std::runtime_error("Error executing LoadRKH as a sub algorithm.");
  }

  //Now get input workspace for this algorithm
  MatrixWorkspace_sptr rkhInput = loadRKH->getProperty("OutputWorkspace");

  //Check that the workspace to rebin has the same units as the one that we are matching to
  MatrixWorkspace_sptr toCorrect = getProperty("WorkspaceToCorrect");
  checkWorkspaceUnits(rkhInput, toCorrect);
  
  //Need to rebin the RKH to the same binning as the workspace we are going to correct
  //using the RebinToWorkspace algorithm
  Algorithm_sptr rebinToWS = createSubAlgorithm("RebinToWorkspace");
  rebinToWS->setProperty("WorkspaceToRebin", rkhInput);
  rebinToWS->setProperty("WorkspaceToMatch", toCorrect);
  rebinToWS->setPropertyValue("OutputWorkspace", "rkhout");
  
  try
  {
    rebinToWS->execute();
  }
  catch(std::runtime_error&)
  {
    g_log.error() << "Unable to run RebinToWorkspace subalgorithm.";
    throw std::runtime_error("Error executing RebinToWorkspace as a sub algorithm.");
  }
  
  MatrixWorkspace_sptr rkhworkspace = rebinToWS->getProperty("OutputWorkspace");

  //Use specified operation to correct
  std::string operation = getProperty("WorkspaceOperation");
  MatrixWorkspace_sptr corrected_ws;
  if( operation == "Divide" )
    corrected_ws = toCorrect / rkhworkspace;
  else
    corrected_ws = toCorrect * rkhworkspace;
  
  //Set the resulting workspace
  setProperty("Outputworkspace", corrected_ws);
}

/**
 * Check whether the units on both workspaces match
 * @param base The workspace to test against
 * @param test The workspace to test units of
 */
void CorrectToFile::checkWorkspaceUnits(MatrixWorkspace_sptr base, MatrixWorkspace_sptr test)
{
  std::string baseUnit = base->getAxis(0)->unit()->unitID();
  if( test->getAxis(0)->unit()->unitID() ==  baseUnit ) return;
  g_log.warning() << "The workspace being corrected does not have the correct units. They will "
		  << "be converted to " << baseUnit << "\n"; 
  //Convert the test workspace in place
  //Run convert units on workspace that we are matching to
  Algorithm_sptr convert = createSubAlgorithm("ConvertUnits");
  convert->setProperty<MatrixWorkspace_sptr>("InputWorkspace", test);
  convert->setProperty("Target", baseUnit);
  convert->setProperty("OutputWorkspace", test);
  
  try
  {
    convert->execute();
  }
  catch(std::runtime_error&)
  {
    g_log.error() << "Unable to run sub-algorithm ConvertUnits.";
    throw std::runtime_error("Error running ConvertUnits as a sub algorithm.");
  }
  test = convert->getProperty("OutputWorkspace");
  g_log.information() << "Units converted successfully.\n";
}

