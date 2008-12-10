//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseByCurrent.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseByCurrent)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& NormaliseByCurrent::g_log = Logger::get("NormaliseByCurrent");

/// Default constructor
NormaliseByCurrent::NormaliseByCurrent() : Algorithm() {}

//Destructor
NormaliseByCurrent::~NormaliseByCurrent() {}

void NormaliseByCurrent::init()
{
  declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));
}

void NormaliseByCurrent::exec()
{
  // Get the input workspace
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  // Get the good proton charge and check it's valid
  const double charge = inputWS->getSample()->getProtonCharge();
  if ( charge < 1.0E-6 )
  {
    g_log.error() << "The proton charge is not set to a valid value. Charge = " << charge << std::endl;
    throw std::out_of_range("The proton charge is not set to a valid value");
  }

  g_log.information() << "Normalisation constant: " << charge << std::endl;
  Workspace_sptr outputWS = inputWS / charge;
  outputWS->setYUnit("Counts per microAmp.hour");
  setProperty("OutputWorkspace",outputWS);
}

} // namespace Algorithm
} // namespace Mantid
