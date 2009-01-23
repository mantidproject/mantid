//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <math.h>

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
        declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));
        declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));
	BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
        zeroOrGreater->setLower(0);    
	declareProperty("XMin",0, zeroOrGreater);
	declareProperty("XMax",0, zeroOrGreater->clone());
	std::vector<std::string> propOptions;
	propOptions.push_back("None");
	propOptions.push_back("Linear");
        declareProperty("Interpolation", "None", new ListValidator(propOptions) );

    }

    /** Executes the algorithm
     *
     */
    void RemoveTimeBins::exec()
    {
	    //Get input workspace and offset
	    MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
	    int start = getProperty("XMin");
	    int end = getProperty("XMax");
	    
	            
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
	    
	    //Get number of histograms
	    int numHists = inputW->getNumberHistograms();

	    if (start != 0 && end != inputW->dataX(0).size() -1)
	    {    
		    //Create new workspace
		    API::MatrixWorkspace_sptr outputWS
		         = API::WorkspaceFactory::Instance().create(inputW, numHists, inputW->dataX(0).size(), inputW->dataY(0).size());
		    
		    //Remove bins from middle
		    RemoveFromMiddle(inputW, outputWS, numHists, start, end);
	    }
	    else
	    {    
		 //Create new workspace
		API::MatrixWorkspace_sptr outputWS
		 = API::WorkspaceFactory::Instance().create(inputW, numHists, inputW->dataX(0).size() - (end - start +1), inputW->dataY(0).size() - (end - start +1));
	        
		//Remove bins from either end
		RemoveFromEnds(inputW, outputWS, numHists, start, end);
	   }

    }
    
    void RemoveTimeBins::RemoveFromEnds(API::MatrixWorkspace_const_sptr inputW, 
		API::MatrixWorkspace_sptr outputWS, int numHists, int start, int end)
    {
	    for (int i=0; i < numHists; ++i)
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
		catch(Exception::IndexError) 
		{
			// OK, so this isn't a Workspace2D
		}
		
		// Assign it to the output workspace property
	        setProperty("OutputWorkspace",outputWS);
    }
    
    void RemoveTimeBins::RemoveFromMiddle(API::MatrixWorkspace_const_sptr inputW, 
		API::MatrixWorkspace_sptr outputWS, int numHists, int start, int end)
	{
		std::string interpolation = getProperty("Interpolation");
	    
		//Remove bins from middle
		double valPrev = 0;
		double valNext = 0;
		double errPrev = 0;
		double errNext = 0;  
			    
		for (int i=0; i < numHists; ++i)
		{
			//Values for interpolation
			if (interpolation == "Linear")
			{
				valPrev = inputW->dataY(i)[start - 1];
				valNext = inputW->dataY(i)[end + 1];
				errPrev = inputW->dataE(i)[start - 1];
				errNext = inputW->dataE(i)[end + 1];
			}
			   
			double ave = (valPrev + valNext)/2; //Currently only does average - will do proper interpolation later
			double aveE = (errPrev + errNext)/2; //Cheat: should add in quadrature will do later
			    
			for (int j=0; j < inputW->dataY(i).size(); ++j)
			{			
				outputWS->dataX(i)[j] = inputW->dataX(i)[j];
				 
				 if (j >= start && j <= end)
				 {					 
					outputWS->dataY(i)[j] = ave;
					outputWS->dataE(i)[j] = aveE;
				 }
				 else
				 {
					outputWS->dataY(i)[j] =  inputW->dataY(i)[j];
					outputWS->dataE(i)[j] =  inputW->dataE(i)[j];
				 }
			}
			    
			//X has one more value
			outputWS->dataX(i)[inputW->dataY(i).size() - 1] = inputW->dataX(i)[inputW->dataY(i).size() -1];
		}	  

		// Copy units
		if (outputWS->getAxis(0)->unit().get())
			outputWS->getAxis(0)->unit() = inputW->getAxis(0)->unit();
		try
		{
			if (inputW->getAxis(1)->unit().get())
				outputWS->getAxis(1)->unit() = inputW->getAxis(1)->unit();
		}
		catch(Exception::IndexError) 
		{
			// OK, so this isn't a Workspace2D
		}
		
		// Assign it to the output workspace property
		setProperty("OutputWorkspace",outputWS);
       }
    
  } // namespace Algorithm
} // namespace Mantid




