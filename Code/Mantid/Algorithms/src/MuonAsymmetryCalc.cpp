//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MuonAsymmetryCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"

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
       declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
	    
       BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
       zeroOrGreater->setLower(0);
       declareProperty("ForwardSpectrum",0,zeroOrGreater );
       declareProperty("BackwardSpectrum",0,zeroOrGreater->clone() );
       declareProperty("Alpha",1.0,Direction::Input);
    }

    /** Executes the algorithm
     *
     */
    void MuonAsymmetryCalc::exec()
    {
	    int forward = getProperty("ForwardSpectrum");		//Start of forward grouping
	    int backward = getProperty("BackwardSpectrum");    //Start of backward grouping
	    double alpha = getProperty("Alpha");
	    
	    //Get original workspace
	    API::Workspace_const_sptr inputWS = getProperty("InputWorkspace");
	    
	    //Make sure backward is greater than forward
	    if (forward > backward)
	    {
		 int temp = forward;
		 forward = backward;
		 backward = temp;
	    }

	    //should we do this check?
	    int numSpectra = inputWS->size() / inputWS->blocksize();
	    
	    if (backward - forward !=  numSpectra - backward)
	    {
		g_log.error("Should number of detectors in the groups match?");
		throw std::invalid_argument("Should number of detectors in the groups match?");
	    }
	    
	    //Create a workspace with only one spectra for forward
	    DataObjects::Workspace2D_sptr forwardWS 
		= boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create(inputWS, 1, inputWS->dataX(0).size(), inputWS->blocksize()));
	    
	    //Create a workspace with only one spectra for backward
	    DataObjects::Workspace2D_sptr backwardWS 
		= boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create(forwardWS));
	    
	    DataObjects::Workspace2D_sptr outputWS 
		= boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create(forwardWS));
	    
	    //Compile the forward and backward spectra 
	    for (int i = forward; i < backward; ++i)
	    {    
		    //Sum up the groups of spectra
		    for (int j = 0; j < inputWS->blocksize(); ++j)
		    {
			    forwardWS->dataY(0)[j] = forwardWS->dataY(0)[j] + inputWS->dataY(i)[j];
			    backwardWS->dataY(0)[j] = backwardWS->dataY(0)[j] + alpha * inputWS->dataY(i + backward)[j];
		    }
	    }
	    
	    //Calculate asymmetry for each time bin
	    for (int j = 0; j < inputWS->blocksize(); ++j)
	    {
		    outputWS->dataY(0)[j] 
			= (forwardWS->dataY(0)[j] - backwardWS->dataY(0)[j])/(forwardWS->dataY(0)[j] + backwardWS->dataY(0)[j]);    
	    }
	    
	    //std::cout << "F = " << forwardWS->dataY(0)[10] << std::endl;
	    //std::cout << "aB = " << backwardWS->dataY(0)[10] << std::endl;
	    //std::cout << "output = " << outputWS->dataY(0)[10] << std::endl;
   
	    setProperty("OutputWorkspace", outputWS);
    }

  } // namespace Algorithm
} // namespace Mantid




