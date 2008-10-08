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
        declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
        declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));
	BoundedValidator<double> *isDouble = new BoundedValidator<double>();
	declareProperty("Offset",0.0,isDouble);
    }

    /** Executes the algorithm
     *
     */
    void ChangeBinOffset::exec()
    {
	    //Get input workspace and offset
	    Workspace_const_sptr inputW = getProperty("InputWorkspace");
	    double offset = getProperty("Offset");
	        
	    //Create new workspace for output from old
	    API::Workspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW);
	    outputW->isDistribution(inputW->isDistribution());
	    
	    //Get number of histograms
	    int histnumber = inputW->getNumberHistograms();
	    
	    for (int i=0; i < histnumber; ++i)
	    {
		    // get const references to input Workspace X data (no copying)
		    const std::vector<double>& XValues = inputW->dataX(i);

		    //get references to output workspace data (no copying)
		    std::vector<double>& XValues_new=outputW->dataX(i);
		    
		    for (int j=0; j < XValues.size(); ++j)
		    {
			    XValues_new[j] = XValues[j] + offset;
		    }
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
    
  } // namespace Algorithm
} // namespace Mantid




