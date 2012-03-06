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
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MuonRemoveExpDecay)

/// Sets documentation strings for this algorithm
void MuonRemoveExpDecay::initDocs()
{
  this->setWikiSummary("This algorithm removes the exponential decay from a muon workspace. ");
  this->setOptionalMessage("This algorithm removes the exponential decay from a muon workspace.");
}


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
  declareProperty(new Kernel::ArrayProperty<int>("Spectra", empty), "Spectra to remove the exponential decay from");
}

/** Executes the algorithm
 *
 */
void MuonRemoveExpDecay::exec()
{
  std::vector<int> Spectra = getProperty("Spectra");

  //Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  int numSpectra = static_cast<int>(inputWS->size() / inputWS->blocksize());

  //Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(inputWS);

  if (Spectra.empty())
  {
    Progress prog(this, 0.0, 1.0, numSpectra);
    //Do all the spectra	
    PARALLEL_FOR2(inputWS,outputWS)
    for (int i = 0; i < numSpectra; ++i)
    {
			PARALLEL_START_INTERUPT_REGION
      removeDecayData(inputWS->readX(i), inputWS->readY(i), outputWS->dataY(i));
      removeDecayError(inputWS->readX(i), inputWS->readE(i), outputWS->dataE(i)); 
      outputWS->dataX(i) = inputWS->readX(i);   

      double normConst = calNormalisationConst(outputWS, i);

      // do scaling and substract by minus 1.0
      for (size_t j = 0; j < outputWS->dataY(i).size(); j++)
      {
        outputWS->dataY(i)[j] /= normConst;
        outputWS->dataY(i)[j] -= 1.0;
        outputWS->dataE(i)[j] /= normConst;
      }   

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
      for (int64_t i = 0; i < int64_t(numSpectra); ++i)
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
    int specLength = static_cast<int>(Spectra.size());
    PARALLEL_FOR2(inputWS,outputWS)
    for (int i = 0; i < specLength; ++i)
    {
			PARALLEL_START_INTERUPT_REGION
      if (Spectra[i] > numSpectra)
      {
        g_log.error("Spectra size greater than the number of spectra!");
        throw std::invalid_argument("Spectra size greater than the number of spectra!");
      }

      removeDecayData(inputWS->readX(Spectra[i]), inputWS->readY(Spectra[i]), outputWS->dataY(Spectra[i]));
      removeDecayError(inputWS->readX(Spectra[i]), inputWS->readE(Spectra[i]), outputWS->dataE(Spectra[i]));
      outputWS->dataX(Spectra[i]) = inputWS->readX(Spectra[i]);

      double normConst = calNormalisationConst(outputWS, Spectra[i]);

      // do scaling and substract by minus 1.0
      for (size_t j = 0; j < outputWS->dataY(i).size(); j++)
      {
        outputWS->dataY(Spectra[i])[j] /= normConst;
        outputWS->dataY(Spectra[i])[j] -= 1.0;
        outputWS->dataE(Spectra[i])[j] /= normConst;
      }   

      prog.report();
			PARALLEL_END_INTERUPT_REGION
    }
		PARALLEL_CHECK_INTERUPT_REGION
  }

  setProperty("OutputWorkspace", outputWS);
}

/** This method corrects the errors for one spectra.
 *	 The muon lifetime is in microseconds not seconds, i.e. 2.2 rather than 0.0000022.
 *   This is because the data is in microseconds.
 *   @param inX ::  The X vector
 *   @param inY ::  The input error vector
 *   @param outY :: The output error vector
 */
void MuonRemoveExpDecay::removeDecayError(const MantidVec& inX, const MantidVec& inY,
    MantidVec& outY)
{
  //Do the removal
  for (size_t i = 0; i < inY.size(); ++i)
  {
    if ( inY[i] )
      outY[i] = inY[i] * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
    else
      outY[i] = 1.0 * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
  }
}

/** This method corrects the data for one spectra.
 *	 The muon lifetime is in microseconds not seconds, i.e. 2.2 rather than 0.0000022.
 *   This is because the data is in microseconds.
 *   @param inX ::  The X vector
 *   @param inY ::  The input data vector
 *   @param outY :: The output data vector
 */
void MuonRemoveExpDecay::removeDecayData(const MantidVec& inX, const MantidVec& inY,
    MantidVec& outY)
{
  //Do the removal
  for (size_t i = 0; i < inY.size(); ++i)
  {
    if ( inY[i] )
      outY[i] = inY[i] * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
    else
      outY[i] = 0.1 * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
  }
}

/**
 * calculate normalisation constant after the exponential decay has been removed
 * to a linear fitting function
 * @param ws ::  workspace
 * @param wsIndex :: workspace index
 * @return normalisation constant
*/
double MuonRemoveExpDecay::calNormalisationConst(API::MatrixWorkspace_sptr ws, int wsIndex)
{
  double retVal = 1.0; 

    API::IAlgorithm_sptr fit;
    fit = createSubAlgorithm("Fit", -1, -1, true);

    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("WorkspaceIndex", wsIndex);

    std::stringstream ss;
    ss << "name=LinearBackground,A0=" << ws->readY(wsIndex)[0] << ",A1=" << 0.0;
    std::string function = ss.str();

    fit->setPropertyValue("Function", function);
    fit->setProperty("Ties", "A1=0.0");
    fit->execute();

    std::string fitStatus = fit->getProperty("OutputStatus");
    std::vector<double> params = fit->getProperty("Parameters");
    std::vector<std::string> paramnames = fit->getProperty("ParameterNames");

    // Check order of names
    if (paramnames[0].compare("A0") != 0)
    {
      g_log.error() << "Parameter 0 should be A0, but is " << paramnames[0] << std::endl;
      throw std::invalid_argument("Parameters are out of order @ 0, should be A0");
    }
    if (paramnames[1].compare("A1") != 0)
    {
      g_log.error() << "Parameter 1 should be A1, but is " << paramnames[1]
            << std::endl;
      throw std::invalid_argument("Parameters are out of order @ 0, should be A1");
    }

    if (!fitStatus.compare("success"))
    {
      const double A0 = params[0];

      if ( A0 < 0 ) 
      {
        g_log.warning() << "When trying to fit Asymmetry normalisation constant this constant comes out negative." 
          << "To proceed Asym norm constant set to 1.0\n";
      }
      else
      {
        retVal = A0;
      }
    } 
    else
    {
      g_log.warning() << "Fit falled. Status = " << fitStatus << std::endl
                        << "For workspace index " << wsIndex << std::endl
                     << "Asym norm constant set to 1.0\n";
    } 

  return retVal;
}


} // namespace Algorithm
} // namespace Mantid


