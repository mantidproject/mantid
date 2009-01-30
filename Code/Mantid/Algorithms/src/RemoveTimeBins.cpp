//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <math.h>
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
  declareProperty("XMin",0.0);
	declareProperty("XMax",0.0);
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
  double startX = getProperty("XMin");
  double endX = getProperty("XMax");
  unsigned int start=0;
  unsigned int end=0;
	    
  if (startX > endX)
  {
    double temp = startX;
    startX = endX;
    endX = temp;
  }
	    
  //Convert X values into bin numbers
  for (unsigned int i=0; i < inputW->dataX(0).size() ; ++i)
  {
    if (i == 0 && inputW->dataX(0)[i]  > startX)
    {
      start = 0;
      break;
    }
    else if (i == inputW->dataX(0).size() -1)
    {
      start = i;
      break;
    }
    else if (startX >= inputW->dataX(0)[i] && startX < inputW->dataX(0)[i + 1] )
    {
      start = i;
      break;
    }    
  }
	    
  for (unsigned int i=0; i < inputW->dataX(0).size() ; ++i)
  {
    if (i == 0 && inputW->dataX(0)[i]  > endX)
    {
      end = 0;
      break;
    }
    else if (i == inputW->dataX(0).size() -1)
    {
      end = i;
      break;
    }
    else if (endX >= inputW->dataX(0)[i] && endX < inputW->dataX(0)[i + 1] )
    {
      end = i;
      break;
    }
  }		   

  //Check end does not exceed number of time bins
  if (end > inputW->dataX(0).size() -1)
  {
    end = inputW->dataX(0).size() -1;
  }

  //Get number of histograms
  int numHists = inputW->getNumberHistograms();
  
  API::MatrixWorkspace_sptr outputWS;
  
  if (start != 0 && end != inputW->dataX(0).size() -1)
  {    
    //Create new workspace
    outputWS = API::WorkspaceFactory::Instance().create(inputW);
		    
    //Remove bins from middle
    RemoveFromMiddle(inputW, outputWS, numHists, start, end);
  }
  else
  {    
    //Create new workspace
    outputWS = API::WorkspaceFactory::Instance().create(inputW, numHists, inputW->dataX(0).size() - (end - start +1), inputW->dataY(0).size() - (end - start +1));
	        
    //Remove bins from either end
    RemoveFromEnds(inputW, outputWS, numHists, start, end);
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWS);
}
    
void RemoveTimeBins::RemoveFromEnds(API::MatrixWorkspace_const_sptr inputW, 
    API::MatrixWorkspace_sptr outputWS, int numHists, unsigned int start, unsigned int end)
{
  // If this is a Workspace2D, get the spectra axes for copying in the spectraNo later
  Axis *specAxis = NULL, *outAxis = NULL;
  if (inputW->axes() > 1)
  {
    specAxis = inputW->getAxis(1);
    outAxis = outputWS->getAxis(1);
  }

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
      loopEnd = start - 1;
    }

    for (int j = loopStart; j < loopEnd; ++j)
    {
      outputWS->dataX(i)[count] = inputW->dataX(i)[j];
      outputWS->dataY(i)[count] = inputW->dataY(i)[j];
      outputWS->dataE(i)[count] = inputW->dataE(i)[j];

      ++count;
    }
    //X has one more value
    outputWS->dataX(i)[count] = inputW->dataX(i)[loopEnd];

    // Copy spectrum number
    if (specAxis) outAxis->spectraNo(i) = specAxis->spectraNo(i);

  }
				
}
    
void RemoveTimeBins::RemoveFromMiddle(API::MatrixWorkspace_const_sptr inputW, 
    API::MatrixWorkspace_sptr outputWS, int numHists, unsigned int start, unsigned int end)
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

    double m = (valNext - valPrev)/(1.0*(end - start) + 2.0); //Gradient
    double c = valPrev; //Intercept

    double aveE = (errPrev + errNext)/2; //Cheat: will do properly later

    for (unsigned int j=0; j < inputW->dataY(i).size(); ++j)
    {			
      outputWS->dataX(i)[j] = inputW->dataX(i)[j];

      if (j >= start && j <= end)
      {					 
        outputWS->dataY(i)[j] = m * (j - start + 1) + c;
        outputWS->dataE(i)[j] = aveE;
      }
      else
      {
        outputWS->dataY(i)[j] =  inputW->dataY(i)[j];
        outputWS->dataE(i)[j] =  inputW->dataE(i)[j];
      }
    }

    //X has one more value
    outputWS->dataX(i)[inputW->dataX(i).size() - 1] = inputW->dataX(i)[inputW->dataX(i).size() -1];
  }	  

}
    
} // namespace Algorithm
} // namespace Mantid
