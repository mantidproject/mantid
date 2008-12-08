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
	    if (end > inputW->dataX(0).size())
	    {
		    end = inputW->dataX(0).size();
	    }
	    
	    if (start > end)
	    {
		    int temp = start;
		    start = end;
		    end = temp;
	    }
	    
	    //Get number of histograms
	    int histnumber = inputW->getNumberHistograms();
	    
	    //Create new workspace
	    API::Workspace_sptr outputWS 
		= API::WorkspaceFactory::Instance().create(inputW, histnumber, inputW->dataX(0).size() - (end - start +1), inputW->blocksize());
	    
	    std::cout << "Output size should be " << inputW->dataX(0).size() - (end - start +1) << std::endl;
	    std::cout << "but is " << outputWS->dataX(0).size() << std::endl;
	    
	    for (int i=0; i < histnumber; ++i)
	    {        
		    int count = 0;
		    
		    for (int j=0; j < inputW->dataX(i).size(); ++j)
		    {
			    if (j >= start and j <= end)
			    {
				    //Do nothing as these are discarded
				    //May be in the future we will want to put them
				    //in a separate workspace?
			    }
			    else
			    {
				    outputWS->dataX(i)[count] = inputW->dataX(i)[j];
				    outputWS->dataY(i)[count] = inputW->dataY(i)[j];
				    outputWS->dataE(i)[count] = inputW->dataE(i)[j];
				    
				    ++count;
				}    
		    }
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




