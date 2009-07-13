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
   using API::Progress;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MuonAsymmetryCalc)

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void MuonAsymmetryCalc::init()
    {
       declareProperty(
         new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
         "Name of the input workspace" );
       declareProperty(
         new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
         "The name of the workspace to be created as the output of the algorithm" );
	    
       BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
       zeroOrGreater->setLower(0);
       declareProperty("ForwardSpectra", 0,
         "The detector number of the forward group (default 0)", Direction::Input);	   
       declareProperty("BackwardSpectra", 1,
         "The detector number of the backward group (default 1)", Direction::Input);		       
       declareProperty("Alpha", 1.0, "The balance parameter (default 1)",
         Direction::Input);
    }

    /** Executes the algorithm
     *
     */
    void MuonAsymmetryCalc::exec()
    {
	    int forward = getProperty("ForwardSpectra");		
	    int backward = getProperty("BackwardSpectra");   
	    double alpha = getProperty("Alpha");
	    
	    //Get original workspace
	    API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
	    
	    //Create a workspace with only one spectra for forward
	    API::MatrixWorkspace_sptr outputWS 
					= API::WorkspaceFactory::Instance().create(inputWS, 1, inputWS->readX(0).size(), inputWS->blocksize());
	    	    
	    //Calculate asymmetry for each time bin
	    //F-aB / F+aB
		Progress prog(this,0.0,1.0,inputWS->blocksize());
	    for (int j = 0; j < inputWS->blocksize(); ++j)
	    {
		    double numerator = inputWS->dataY(forward)[j] - alpha * inputWS->dataY(backward)[j];
		    double denominator = (inputWS->dataY(forward)[j] + alpha * inputWS->dataY(backward)[j]);
		    
            outputWS->dataY(0)[j] = denominator? numerator/denominator : 0.;

		    //Work out the errors	
		    // Note: the error for F-aB = the error for F+aB
		    double quadrature = sqrt( pow(inputWS->dataE(forward)[j], 2) + pow(inputWS->dataE(backward)[j], 2));
		    
            double ratio = numerator && denominator? sqrt( pow(quadrature/numerator, 2) + pow(quadrature/denominator, 2)) : 0.;
		    
		    outputWS->dataE(0)[j] =  ratio * outputWS->dataY(0)[j];
			prog.report();
	    }
	    
	    //Copy the imput time bins on to the output
	    outputWS->dataX(0) = inputWS->readX(0);
   
	    setProperty("OutputWorkspace", outputWS);
    }

  } // namespace Algorithm
} // namespace Mantid




