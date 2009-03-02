//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidAlgorithms/ChangeBinOffset.h"

namespace Mantid
{
  namespace Algorithms
  {

   using namespace Kernel;
   using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(ChangeBinOffset)

    // Get a reference to the logger
    Logger& ChangeBinOffset::g_log = Logger::get("ChangeBinOffset");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void ChangeBinOffset::init()
    {
        declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));
        declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));
	BoundedValidator<double> *isDouble = new BoundedValidator<double>();
	declareProperty("Offset",0.0,isDouble);
    }

    /** Executes the algorithm
     *
     */
    void ChangeBinOffset::exec()
    {
	    //Get input workspace and offset
	    const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
	    double offset = getProperty("Offset");
	    
	    API::MatrixWorkspace_sptr outputW = createOutputWS(inputW);	    
	    
	    //Get number of histograms
	    int histnumber = inputW->getNumberHistograms();
	    
	    for (int i=0; i < histnumber; ++i)
	    {		    
		    //Do the offsetting
		    for (int j=0; j <  inputW->readX(i).size(); ++j)
		    {
			    //Change bin value by offset
			    outputW->dataX(i)[j] = inputW->readX(i)[j] + offset;
		    }
		    
		    //Copy y and e data
		    outputW->dataY(i) = inputW->dataY(i);
		    outputW->dataE(i) = inputW->dataE(i);
	    }
	    
	    // Copy units
            if (outputW->getAxis(0)->unit().get())
                outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
            try
            {
                if (inputW->getAxis(1)->unit().get())
                    outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
            }
            catch(Exception::IndexError) {
                // OK, so this isn't a Workspace2D
            }

	    // Assign it to the output workspace property
	    setProperty("OutputWorkspace",outputW);
    }
    
    API::MatrixWorkspace_sptr ChangeBinOffset::createOutputWS(API::MatrixWorkspace_sptr input)
   {
	   //Check whether input = output to see whether a new workspace is required.
	    if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"))
	    {
		    //Overwrite the original
		    return input;
	    }
	    else
	    {	    
		//Create new workspace for output from old
		API::MatrixWorkspace_sptr output = API::WorkspaceFactory::Instance().create(input);
		output->isDistribution(input->isDistribution());
		return output;
	    }
    }	
    
  } // namespace Algorithm
} // namespace Mantid




