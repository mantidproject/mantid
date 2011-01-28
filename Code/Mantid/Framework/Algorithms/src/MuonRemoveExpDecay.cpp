//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidAPI/Workspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/MuonRemoveExpDecay.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MuonRemoveExpDecay)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MuonRemoveExpDecay::init()
{
  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "",
      Direction::Input), "Name of the input 2D workspace");
  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
      Direction::Output), "The name of the workspace to be created as the output of the algorithm");
  std::vector<int> empty;
  declareProperty(new Kernel::ArrayProperty<int>("Spectra", empty, new MandatoryValidator<std::vector<
      int> > ), "Spectra to remove the exponential decay from");
}

/** Executes the algorithm
 *
 */
void MuonRemoveExpDecay::exec()
{
  std::vector<int> Spectra = getProperty("Spectra");

  //Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  int numSpectra = inputWS->size() / inputWS->blocksize();

  //Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(inputWS);

  if (Spectra.size() == 0)
  {
    Progress prog(this, 0.0, 1.0, numSpectra);
    //Do all the spectra	
    PARALLEL_FOR2(inputWS,outputWS)
    for (int i = 0; i < numSpectra; ++i)
    {
			PARALLEL_START_INTERUPT_REGION
      removeDecay(inputWS->readX(i), inputWS->readY(i), outputWS->dataY(i));
      outputWS->dataX(i) = inputWS->readX(i);

      //Need to do something about the errors?
      prog.report();
			PARALLEL_END_INTERUPT_REGION
    }
		PARALLEL_CHECK_INTERUPT_REGION
  }
  else
  {
    Progress prog(this, 0.0, 1.0, numSpectra + Spectra.size());
    if (getPropertyValue("InputWorkspace") != getPropertyValue("OutputWorkspace"))
    {

      //Copy all the X,Y and E data
      PARALLEL_FOR2(inputWS,outputWS)
      for (int i = 0; i < numSpectra; ++i)
      {
				PARALLEL_START_INTERUPT_REGION
        outputWS->dataX(i) = inputWS->readX(i);
        outputWS->dataY(i) = inputWS->readY(i);
        outputWS->dataE(i) = inputWS->readE(i);
        prog.report();
				PARALLEL_END_INTERUPT_REGION
      }
			PARALLEL_CHECK_INTERUPT_REGION
    }

    //Do the specified spectra only
    int specLength = Spectra.size();
    PARALLEL_FOR2(inputWS,outputWS)
    for (int i = 0; i < specLength; ++i)
    {
			PARALLEL_START_INTERUPT_REGION
      if (Spectra[i] > numSpectra)
      {
        g_log.error("Spectra size greater than the number of spectra!");
        throw std::invalid_argument("Spectra size greater than the number of spectra!");
      }

      removeDecay(inputWS->readX(Spectra[i]), inputWS->readY(Spectra[i]), outputWS->dataY(Spectra[i]));
      outputWS->dataX(Spectra[i]) = inputWS->readX(Spectra[i]);

      //Need to do something about the errors?
      prog.report();
			PARALLEL_END_INTERUPT_REGION
    }
		PARALLEL_CHECK_INTERUPT_REGION
  }

  setProperty("OutputWorkspace", outputWS);
}

/** This method corrects the data for one spectra.
 *	 The muon lifetime is in microseconds not seconds, i.e. 2.2 rather than 0.0000022.
 *   This is because the data is in microseconds.
 *   @param inX ::  The X vector
 *   @param inY ::  The input data vector
 *   @param outY :: The output data vector
 */
void MuonRemoveExpDecay::removeDecay(const MantidVec& inX, const MantidVec& inY,
    MantidVec& outY)
{
  //Do the removal
  for (size_t i = 0; i < inY.size(); ++i)
  {
    outY[i] = inY[i] * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
  }
}

} // namespace Algorithm
} // namespace Mantid


