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

void NormaliseByCurrent::exec()
{
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Get the good proton charge and check it's valid
  //double charge = inputWS->getSample()->getProtonCharge();
  double charge = inputWS->sample().getProtonCharge();
  if ( charge < 1.0E-6 )
  {
    g_log.error() << "The proton charge is not set to a valid value. Charge = " << charge << std::endl;
    throw std::out_of_range("The proton charge is not set to a valid value");
  }

  g_log.information() << "Normalisation current: " << charge << " uamps" <<  std::endl;

  charge=1.0/charge; // Inverse of the charge to be multiplied by

  if (inputWS==outputWS) //
    {
//  	  Workspace::iterator it(*inputWS);
//  	  for (;it!=it.end();++it)
//  	  {
//  		  ((*it).Y())*=charge;
//  		  ((*it).E())*=charge;
//  	  }
	  const int nspec=inputWS->getNumberHistograms()-1;
	  const int nbin=inputWS->blocksize()-1;
	  m_progress =new Progress(this,0.0,1.0,nspec);
	   for (int i=nspec;i>=0;--i)
	  {
		  MantidVec& refY=inputWS->dataY(i);
		  MantidVec& refE=inputWS->dataE(i);
		  for (int j=nbin;j>=0;--j)
		  {
			 refY[j]*=charge;
			 refE[j]*=charge;
		  }
		  m_progress->report();
	  }
    }
  else
  {
	  outputWS = inputWS*charge;
    setProperty("OutputWorkspace",outputWS);
  }

  outputWS->setYUnit("Counts per microAmp.hour");
}

} // namespace Algorithm
} // namespace Mantid
