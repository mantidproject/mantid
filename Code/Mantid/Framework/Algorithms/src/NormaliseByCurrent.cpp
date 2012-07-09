/*WIKI* 

Normalises a workspace according to the good proton charge figure taken from the Input Workspace log data, which is stored in the workspace's [[Sample]] object). Every data point (and its error) is divided by that number.

== ISIS Calculation Details ==
The good proton charge '''gd_ptrn_chrg''' is an summed value that applies across all periods. It is therefore suitable to run NormaliseByProtonCharge for single-period workspaces, but gives incorrect normalisation for multi-period workspaces. If the algorithm detects the presences of a multi-period workspace, it calculates the normalisation slightly differently. It uses the '''current_period''' log property to index into the '''proton_charge_by_period''' log data array property.

=== EventWorkspaces ===
If the input workspace is an [[EventWorkspace]], then the output will be as well. Weighted events are used to scale by the current (see the [[Divide]] algorithm, which is a subalgorithm being used).

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseByCurrent)

/// Sets documentation strings for this algorithm
void NormaliseByCurrent::initDocs()
{
  this->setWikiSummary(" Normalises a workspace by the proton charge. ");
  this->setOptionalMessage("Normalises a workspace by the proton charge.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Default constructor
NormaliseByCurrent::NormaliseByCurrent() : Algorithm() {}

//Destructor
NormaliseByCurrent::~NormaliseByCurrent() {}

void NormaliseByCurrent::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "Name of the input workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace");
}

/**
Extract a value for the charge from the input workspace. Handles either single period or multi-period data.
@param inputWS : The input workspace to extract the log details from.
*/
double NormaliseByCurrent::extractCharge(MatrixWorkspace_sptr inputWS) const
{
  // Get the good proton charge and check it's valid
  double charge(-1.0);
  const Run& run = inputWS->run();

  int nPeriods = 0;
  try
  {
    Property* nPeriodsProperty = run.getLogData("nperiods");
    Kernel::toValue<int>(nPeriodsProperty->value(), nPeriods);
  }
  catch(Exception::NotFoundError &)
  {
    g_log.warning() << "No nperiods property. If this is multi-period data, then you will be normalising against the wrong current.\n";
  } 
  // Handle multiperiod data.
  if(nPeriods > 0)
  {
    // Fetch the period property
    Property* currentPeriodNumberProperty = run.getLogData("current_period");
    int periodNumber = atoi(currentPeriodNumberProperty->value().c_str());

    // Fetch the charge property
    Property* chargeProperty = run.getLogData("proton_charge_by_period");
    ArrayProperty<double>* chargePropertyArray = dynamic_cast<ArrayProperty<double>* >(chargeProperty);
    charge = chargePropertyArray->operator()()[periodNumber-1];
  }
  else
  {
    try
    {
      charge = inputWS->run().getProtonCharge();
    }
    catch(Exception::NotFoundError &)
    {
      g_log.error() << "The proton charge is not set for the run attached to this workspace\n";
      throw;
    }
  }
  return charge;
}

void NormaliseByCurrent::exec()
{
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Get the good proton charge and check it's valid
  double charge = extractCharge(inputWS);

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
