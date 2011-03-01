//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseByCurrent)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Default constructor
NormaliseByCurrent::NormaliseByCurrent() : Algorithm() {}

//Destructor
NormaliseByCurrent::~NormaliseByCurrent() {}

void NormaliseByCurrent::init()
{
  //this->setWikiSummary("Normalises a workspace by the proton charge.");
  //this->setOptionalMessage("Normalises a workspace by the proton charge.");

  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "Name of the input workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace");
}

void NormaliseByCurrent::exec()
{
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Get the good proton charge and check it's valid
  double charge(-1.0);
  try
  {
    charge = inputWS->run().getProtonCharge();
  }
  catch(Exception::NotFoundError &)
  {
    g_log.error() << "The proton charge is not set for the run attached to this worksapce\n";
    throw;
  }

  if (charge == 0)
  {
    throw std::domain_error("The proton charge is zero");
  }

  g_log.information() << "Normalisation current: " << charge << " uamps" <<  std::endl;

  charge=1.0/charge; // Inverse of the charge to be multiplied by

  // The operator overloads properly take into account of both EventWorkspaces and doing it in place or not.

  if (getPropertyValue("InputWorkspace") != getPropertyValue("OutputWorkspace"))
  {
    outputWS = inputWS*charge;
    setProperty("OutputWorkspace", outputWS);
  }
  else
  {
    inputWS *= charge;
    setProperty("OutputWorkspace", inputWS);
  }

  outputWS->setYUnitLabel("Counts per microAmp.hour");
}

} // namespace Algorithm
} // namespace Mantid
