//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <math.h>
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

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MuonRemoveExpDecay)

    // Get a reference to the logger
    Logger& MuonRemoveExpDecay::g_log = Logger::get("MuonRemoveExpDecay");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void MuonRemoveExpDecay::init()
    {
       declareProperty(new API::WorkspaceProperty<API::Workspace>("InputWorkspace","",Direction::Input));
       declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace","",Direction::Output));
       std::vector<int> empty;
	declareProperty(new Kernel::ArrayProperty<int>("Spectra", empty,new MandatoryValidator<std::vector<int> >));
    }

    /** Executes the algorithm
     *
     */
    void MuonRemoveExpDecay::exec()
    {
	   std::vector<int> Spectra = getProperty("Spectra");
	    
	    //Get original workspace
	    API::Workspace_const_sptr inputWS = getProperty("InputWorkspace");
	    
	    int numSpectra = inputWS->size() / inputWS->blocksize();
	    
	    //Create output workspace with same dimensions as input
	    API::Workspace_sptr outputWS
		= API::WorkspaceFactory::Instance().create(inputWS);

	    
	    if (Spectra.size() == 0)
	    {
		    //Do all the spectra	    
		    for (int i=0; i < numSpectra; ++i)
		    {
			    removeDecay(inputWS->dataX(i), inputWS->dataY(i), outputWS->dataY(i));
			    outputWS->dataX(i) = inputWS->dataX(i);
			    
			    //Need to do something about the errors?
		    }
	    }
	    else
	    {
		    if (getPropertyValue("InputWorkspace") != getPropertyValue("OutputWorkspace"))
		   {
			//Copy all the X,Y and E data
			for (int i=0; i < numSpectra; ++i)
			{
			    outputWS->dataX(i) = inputWS->dataX(i);
			    outputWS->dataY(i) = inputWS->dataY(i);
			    outputWS->dataE(i) = inputWS->dataE(i);
			}  
		    }
		    
		    //Do the specified spectra only
		    for (int i=0; i < Spectra.size(); ++i)
		    {
			    if (Spectra[i] > numSpectra)
				{
					g_log.error("Spectra size greater than the number of spectra!");
					throw std::invalid_argument("Spectra size greater than the number of spectra!");
				}
			    			    
			   removeDecay(inputWS->dataX(Spectra[i]), inputWS->dataY(Spectra[i]), outputWS->dataY(Spectra[i]));
			   outputWS->dataX(Spectra[i]) = inputWS->dataX(Spectra[i]);
			    
			    //Need to do something about the errors?
		    }
	    }
  
	    setProperty("OutputWorkspace", outputWS);
    }
    
     /** This method corrects the data for one spectra.
     *	   The muon lifetime is in microseconds not seconds, i.e. 2.2 rather than 0.0000022.
     *     This is because the data is in microseconds.
     */
    void MuonRemoveExpDecay::removeDecay(const std::vector<double>& inX, const std::vector<double>& inY, std::vector<double>& outY)
    {
	    //Do the removal
	    for (int i=0; i < inY.size(); ++i)
	    {
			outY[i] = inY[i] * exp(inX[i] / (Mantid::PhysicalConstants::MuonLifetime * 1000000.0));
	    }	
    }

  } // namespace Algorithm
} // namespace Mantid




