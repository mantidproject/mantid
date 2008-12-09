//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <math.h>
#include <vector>

#include "MantidAPI/Workspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/MuonAsymmetryCalc.h"

namespace Mantid
{
  namespace Algorithms
  {

   using namespace Kernel;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MuonAsymmetryCalc)

    // Get a reference to the logger
    Logger& MuonAsymmetryCalc::g_log = Logger::get("MuonAsymmetryCalc");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void MuonAsymmetryCalc::init()
    {
       declareProperty(new API::WorkspaceProperty<API::Workspace>("InputWorkspace","",Direction::Input));
       declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace","",Direction::Output));
	    
       BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
       zeroOrGreater->setLower(0);
       declareProperty(new Kernel::ArrayProperty<int>("ForwardSpectra", new MandatoryValidator<std::vector<int> >));	    
       declareProperty(new Kernel::ArrayProperty<int>("BackwardSpectra", new MandatoryValidator<std::vector<int> >));	    
       declareProperty("Alpha",1.0,Direction::Input);
    }

    /** Executes the algorithm
     *
     */
    void MuonAsymmetryCalc::exec()
    {
	    std::vector<int> forward = getProperty("ForwardSpectra");		
	    std::vector<int> backward = getProperty("BackwardSpectra");   
	    double alpha = getProperty("Alpha");
	    
	    //Get original workspace
	    API::Workspace_const_sptr inputWS = getProperty("InputWorkspace");
	    
	    //~ if (forward.size() == 0 && backward.size() ==0)
	    //~ {
		//~ //Use Nexus file groupings if values were no given
		std::vector<int> groupings = getProperty("detector_groups");
	    
		//~ for (int i= 0; i < groupings.size(); ++i)
		//~ {
		    //~ std::cout << groupings[i] << std::endl;
		    //~ if (groupings[i] == "1")
		    //~ {
			    //~ forward.push_back(i);
		    //~ }	
		    //~ else if (groupings[i] == "2")
		    //~ {
			    //~ backward.push_back(i);
		    //~ }	
			
		//~ }
	    
	    //~ }
	    
	    int numSpectra = inputWS->size() / inputWS->blocksize();
	    
	    //should we do this check?
	    if (forward.size() != backward.size())
	    {
		g_log.error("Should number of detectors in the groups match?");
		throw std::invalid_argument("Should number of detectors in the groups match?");
	    }
	    
	    //Create a workspace with only one spectra for forward
	    API::Workspace_sptr forwardWS 
		= API::WorkspaceFactory::Instance().create(inputWS, 1, inputWS->dataX(0).size(), inputWS->blocksize());
	    
	    //Create a workspace with only one spectra for backward
	    API::Workspace_sptr backwardWS 
		= API::WorkspaceFactory::Instance().create(forwardWS);
	    
	    API::Workspace_sptr outputWS 
		= API::WorkspaceFactory::Instance().create(forwardWS);
	    
	    //Compile the forward and backward spectra 
	    //This assumes that the backward group has the same number of detectors as the forward group
	    for (int i = 0; i < forward.size(); ++i)
	    {    
		    //Check spectra numbers are valid
		    if (forward[i] >= numSpectra || forward[i] < 0 || backward[i] >= numSpectra || backward[i] < 0)
		    {
			g_log.error("Invalid detector number entered in group.");
			throw std::invalid_argument("Invalid detector number entered in group.");
		    }
		    
		    //Sum up the groups of spectra
		    for (int j = 0; j < inputWS->blocksize(); ++j)
		    {
			    forwardWS->dataY(0)[j] = forwardWS->dataY(0)[j] + inputWS->dataY(forward[i])[j];
			    backwardWS->dataY(0)[j] = backwardWS->dataY(0)[j] + alpha * inputWS->dataY(backward[i])[j];
			    
			    //Add the errors in quadrature
			    forwardWS->dataE(0)[j] 
				= sqrt(pow(forwardWS->dataE(0)[j], 2) + pow(inputWS->dataE(forward[i])[j], 2));
			    backwardWS->dataE(0)[j] 
				= sqrt(pow(backwardWS->dataE(0)[j], 2) + pow(inputWS->dataE(backward[i])[j], 2));
		    }
	    }
	    
	    //Calculate asymmetry for each time bin
	    //F-aB / F+aB
	    for (int j = 0; j < inputWS->blocksize(); ++j)
	    {
		    double numerator = forwardWS->dataY(0)[j] - backwardWS->dataY(0)[j];
		    double denominator = (forwardWS->dataY(0)[j] + backwardWS->dataY(0)[j]);
		    
		    outputWS->dataY(0)[j] = numerator/denominator;

		    //Work out the errors	
		    // Note: the error for F-aB = the error for F+aB
		    double quadrature = sqrt( pow(forwardWS->dataE(0)[j], 2) + pow(backwardWS->dataE(0)[j], 2));
		    
		    double ratio = sqrt( pow(quadrature/numerator, 2) + pow(quadrature/denominator, 2));
		    
		    outputWS->dataE(0)[j] =  ratio * outputWS->dataY(0)[j];
	    }
	    
	    //Copy the imput time bins on to the output
	    outputWS->dataX(0) = inputWS->dataX(0);
	    
	    //std::cout << "F = " << forwardWS->dataY(0)[100] << std::endl;
	    //std::cout << "aB = " << backwardWS->dataY(0)[100] << std::endl;
	    //std::cout << "output = " << outputWS->dataY(0)[100] << std::endl;
	    //std::cout << "error = " << outputWS->dataE(0)[100] << std::endl;
   
	    setProperty("OutputWorkspace", outputWS);
    }

  } // namespace Algorithm
} // namespace Mantid




