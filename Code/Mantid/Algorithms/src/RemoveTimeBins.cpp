//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidAlgorithms/RemoveTimeBins.h"

namespace Mantid
{
  namespace Algorithms
  {

   using namespace Kernel;
   using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(RemoveTimeBins)

    // Get a reference to the logger
    Logger& RemoveTimeBins::g_log = Logger::get("RemoveTimeBins");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void RemoveTimeBins::init()
    {
        declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
        declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));
	BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
        zeroOrGreater->setLower(0);    
	declareProperty("StartTimeBin",0, zeroOrGreater);
	declareProperty("EndTimeBin",0, zeroOrGreater->clone());
    }

    /** Executes the algorithm
     *
     */
    void RemoveTimeBins::exec()
    {
	    //Get input workspace and offset
	    Workspace_const_sptr inputW = getProperty("InputWorkspace");
	    int start = getProperty("StartTimeBin");
	    int end = getProperty("EndTimeBin");
	            
	    //Check end does not exceed number of time bins
	    if (end > inputW->dataX(0).size() -1)
	    {
		    end = inputW->dataX(0).size() -1;
	    }
	    
	    if (start > end)
	    {
		    int temp = start;
		    start = end;
		    end = temp;
	    }
	    
	    if (start != 0 && end != inputW->dataX(0).size() -1)
	    {    
		    g_log.error("You are trying to remove timebins from middle of workspace, this algorithm is not suitable for that operation.");
		    throw std::invalid_argument("You are trying to remove timebins from middle of workspace, this algorithm is not suitable for that operation.");
	    }
	    
	    //Get number of histograms
	    int histnumber = inputW->getNumberHistograms();
	    
	    //Create new workspace
	    API::Workspace_sptr outputWS
		= API::WorkspaceFactory::Instance().create(inputW, histnumber, inputW->dataX(0).size() - (end - start +1), inputW->dataY(0).size() - (end - start +1));
	        
	    
	    for (int i=0; i < histnumber; ++i)
	    {        
		int count = 0;
		int loopStart;
		int loopEnd;
	    
		if (start == 0)
		{
			//Remove from front
			loopStart = end + 1;
			loopEnd = inputW->dataY(i).size();
		}
		else
		{
			//Remove from end
			loopStart = 0;
			loopEnd = start -1;
		}
		        
		for (int j=loopStart; j < loopEnd; ++j)
		{			
			outputWS->dataX(i)[count] = inputW->dataX(i)[j];
			outputWS->dataY(i)[count] = inputW->dataY(i)[j];
			outputWS->dataE(i)[count] = inputW->dataE(i)[j];
				    
			++count;   
		}
		//X has one more value
		outputWS->dataX(i)[count] = inputW->dataX(i)[loopEnd];		    		    
	    }
	    
	    // Copy units
            if (outputWS->getAxis(0)->unit().get())
                outputWS->getAxis(0)->unit() = inputW->getAxis(0)->unit();
            try
            {
                if (inputW->getAxis(1)->unit().get())
                    outputWS->getAxis(1)->unit() = inputW->getAxis(1)->unit();
            }
            catch(Exception::IndexError) {
                // OK, so this isn't a Workspace2D
            }

	    // Assign it to the output workspace property
	    setProperty("OutputWorkspace",outputWS);
    }
    
  } // namespace Algorithm
} // namespace Mantid




