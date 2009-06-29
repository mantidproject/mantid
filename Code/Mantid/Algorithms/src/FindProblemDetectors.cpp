//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindProblemDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <string>
#include <vector>
#include <fstream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindProblemDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& FindProblemDetectors::g_log = Logger::get("FindProblemDetectors");

void FindProblemDetectors::init()
{
  //STEVE update the wiki
  HistogramValidator<MatrixWorkspace> *val =
    new HistogramValidator<MatrixWorkspace>;
  declareProperty(
    new WorkspaceProperty<>("WhiteBeamWorkspace","",Direction::Input,val),
    "Name of a white beam vanadium workspace" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Each histogram from the input workspace maps to a histogram in this\n"
    "workspace with one value that indicates if there was a dead detector" );

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0);
  //STEVES should users beable to set the failure labels for high and low?
  declareProperty("LowThreshold", 0.1, mustBePositive,
    "Detectors with signals less than this proportion of the median\n"
    "value will be labeled as reading badly (default 0.1)" );
  declareProperty("HighThreshold", 1.5, mustBePositive->clone(),
    "Detectors with signals more than this number of times the median\n"
    "signal will be labeled as reading badly (default 1.5)" );
/*STEVES Allow users to change the failure codes?      declareProperty("LiveValue", 0.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'live'\n"
        "(default 0.0)");
      declareProperty("DeadValue", 100.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'dead'\n"
        "(default 100.0)" );*/
  BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
  mustBePosInt->setLower(0);
  declareProperty("StartSpectrum", 0, mustBePosInt,
    "The index number of the first spectrum to include in the calculation\n"
    "(default 0)" );
  //UNSETINT and EMPTY_DBL() are tags that indicate that no value has been set and we want to use the default
  declareProperty("EndSpectrum", UNSETINT, mustBePosInt->clone(),
    "The index number of the last spectrum to include in the calculation\n"
    "(default 0)" );
  declareProperty("Range_lower", EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be used\n"
    "in the summation that decides if a detector is 'dead' (default: the\n"
    "start of each histogram)" );
  declareProperty("Range_upper", EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be used in the summation that decides if a detector is 'dead'\n"
    "(default: the end of each histogram)" );
  declareProperty("OutputFile","",
    "A filename to which to write the list of dead detector UDETs (default\n"
    "no writing to file)" );
      // This output property will contain the list of UDETs for the dead detectors
  declareProperty("FoundDead",std::vector<int>(),Direction::Output);
}

/** Executes the algorithm that includes calls to SolidAngle and Integration
*
*  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*  @throw runtime_error if algorithm cannot execute
*/
void FindProblemDetectors::exec()
{
  //gets and checks the values passed to the algorithm that need checking, throws an invalid_argument if we can't find a good value for a property
  retrieveProperties();
  //now do the calculations ...  
  //calls subalgorithm SolidAngle to get the total solid angle of the detectors that acquired each spectrum or nul on failure
  MatrixWorkspace_sptr angles = getSolidAngles( m_InputWS, m_MinSpec, m_MaxSpec);
  //Adds the counts from all the bins and puts them in one total bin, calls subalgorithm Integration
  MatrixWorkspace_sptr counts = getTotalCounts( m_InputWS, m_MinSpec, m_MaxSpec/*the range properties are also read by this function*/);
  //Divides the total number of counts by the number of seconds, calls the ConvertToDistribution algorithm
  counts = getRate(counts);
  //Gets the count rate per solid angle (in steradians), if it exists, for each spectrum
  //this calculation is optional, it depends on angle information existing
  if ( angles.use_count() == 1 )
  {
    //we are going to correct for solid angle using division. OK let's remove any histograms with zero angle
    //rejectZeros(angles);
    counts = counts/angles;     
  }
  //Gets an average of the data, the medain is less influenced by a small number of huge values than the mean
  double av = getMedian(counts);
  //The final piece of the calculation, remove any detectors whoses signals are outside the threshold range
  std::vector<int> outArray =
    markDetects( counts, av*m_Low, av*m_High, getProperty("OutputFile") );
  
  // Now the calculation is complete, setting the output property to the workspace will register it in the Analysis Data Service and allow the user to see it
  setProperty("OutputWorkspace", counts);

  setProperty("FoundDead", outArray);
}

/** Loads and checks the values passed to the algorithm
*
*  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*/
void FindProblemDetectors::retrieveProperties()
{
  m_InputWS = getProperty("WhiteBeamWorkspace");
  int maxSpecIndex = m_InputWS->getNumberHistograms() - 1;

  m_MinSpec = getProperty("StartSpectrum");
  if ( (m_MinSpec < 0) || (m_MinSpec > maxSpecIndex) )
  {
    g_log.warning("StartSpectrum out of range, changed to 0");
    m_MinSpec = 0;
  }
  m_MaxSpec = getProperty("EndSpectrum");
  if (m_MaxSpec == UNSETINT) m_MaxSpec = maxSpecIndex;
  if ( (m_MaxSpec < 0) || (m_MaxSpec > maxSpecIndex ) )
  {
    g_log.warning("EndSpectrum out of range, changed to max spectrum number");
    m_MaxSpec = maxSpecIndex;
  }
  if ( (m_MaxSpec < m_MinSpec) )
  {
    g_log.warning("EndSpectrum can not be less than the StartSpectrum, changed to max spectrum number");
    m_MaxSpec = maxSpecIndex;
  }

  m_Low = getProperty("LowThreshold");
  m_High = getProperty("HighThreshold");
  if ( !(m_Low < m_High) )
  {
    throw std::invalid_argument("The threshold for reading high must be greater than the low threshold");
  }
}
/*STEVES Allow users to change the failure codes?           
      m_liveValue = getProperty("LiveValue");
      m_deadValue = getProperty("DeadValue");
}*/

/* Makes a worksapce with the total solid angle all the detectors in each spectrum cover from the sample
*  note returns an empty shared pointer on failure, uses the SolidAngle algorithm
* @param input A pointer to a workspace
* @return A share pointer to the workspace (or an empty pointer)
*/
MatrixWorkspace_sptr FindProblemDetectors::getSolidAngles(
                  MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information("Calculating soild angles");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetSolidAngle);
  IAlgorithm_sptr childAlg = createSubAlgorithm("SolidAngle", t0, t1);
  childAlg->setProperty( "InputWorkspace", input );
  childAlg->setProperty( "StartSpectrum", firstSpec );
  childAlg->setProperty( "EndSpectrum", lastSpec );
  try
  {
  // Execute the sub-algorithm, it could throw a runtime_error at this point which would abort execution
    childAlg->execute();
    if ( ! childAlg->isExecuted() )
    {
      throw std::runtime_error("The SolidAngle algorithm executed unsuccessfully");
    }
  }
  //any exception causes an empty workspace pointer to be returned which must be handled by the calling function
  catch(std::exception e)
  {
    g_log.warning(
      "Precision warning:  Can't find detector geometry " + name() +
      " will continue with the solid angles of all spectra set to the same value" );
    failProgress(RTGetSolidAngle);
    MatrixWorkspace_sptr empty;
    return empty;
  }
  return childAlg->getProperty("OutputWorkspace");
}

/* Runs Integration as a sub-algorithm to get the sum of counts in the 
* range specfied by the properties Range_lower and Range_upper
*
* @return Each histogram in the workspace has a single bin containing the sum of the bins in the input workspace
*/
MatrixWorkspace_sptr FindProblemDetectors::getTotalCounts(
                  MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information() << "Integrating input workspace" << std::endl;
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );
  childAlg->setProperty<MatrixWorkspace_sptr>( "InputWorkspace", input );
  childAlg->setProperty( "StartSpectrum", firstSpec );
  childAlg->setProperty( "EndSpectrum", lastSpec );
  // pass inputed values straight to this integration, checking must be done there
  childAlg->setPropertyValue( "Range_lower",  getPropertyValue("Range_lower") );
  childAlg->setPropertyValue( "Range_upper", getPropertyValue("Range_upper") );
  try
  {
    // Now execute integrate
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Exception thrown while running the Integration sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("The Integration algorithm failed unexpectedly, aborting.");
    throw std::runtime_error(name() + " failed trying to run Integration");
  }
  return childAlg->getProperty("OutputWorkspace");
}

/* Divides number of counts by time using ConvertToDistribution as a sub-algorithm
*
* @param counts A histogram workspace with counts in time bins 
* @return A workspace of the counts per unit time in each bin
*/
MatrixWorkspace_sptr FindProblemDetectors::getRate(MatrixWorkspace_sptr counts)
{
  g_log.information("Calculating time averaged count rates");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetRate);
  IAlgorithm_sptr childAlg = createSubAlgorithm("ConvertToDistribution", t0, t1);
  try
  {
    //pass inputed values straight to this sub-algorithm, checking must be done there
    childAlg->setProperty<MatrixWorkspace_sptr>( "Workspace", counts );    
    // Now execute the sub-algorithm. Catch and log any error
    childAlg->execute();
  }
  catch (...)
  {
    g_log.error("Exception thrown while running ConvertToDistribution");
    throw;
  }
  if ( ! childAlg->isExecuted() )
  {
    g_log.error("The ConvertToDistribution algorithm failed unexpectedly, aborting.");
    throw std::runtime_error(name() + " failed to run ConvertToDistribution");
  }
  return childAlg->getProperty("Workspace");
}
/* Checks though the solid angle information and masks detectors in histograms linked to zero or negative solida angle info
*
* @param angles a workspace with single bin histograms of angles
* @throw runtime_error If a solid angle with a negative value is found
*/
void FindProblemDetectors::rejectZeros( API::MatrixWorkspace_const_sptr angles ) const
{
//STEVE to implement this function
}

/* Finds the median of the values in single bin histograms
*
* @param numbers A histogram workspace with one entry in each bin
* @return The median value of the histograms in the workspace that was passed to it
*/
double FindProblemDetectors::getMedian(API::MatrixWorkspace_const_sptr numbers) const
{
  g_log.information("Calculating the median of spectra count rates");
  //make an array for all the single value in each histogram
  boost::scoped_array<double> nums( new double[numbers->getNumberHistograms()] );
  //copy the data into this array
  for (int i = 0; i < numbers->getNumberHistograms(); ++i)
  {
    nums[i] = numbers->readY(i)[0];
  }
  //we need a sorted array to calculate the median
  gsl_sort( nums.get(), 1, numbers->getNumberHistograms() );
  return gsl_stats_median_from_sorted_data
                            ( nums.get(), 1, numbers->getNumberHistograms() );
}
/* Takes a single valued histogram workspace and assesses which histograms are within the limits
*
* @param responses a workspace of histograms with one bin
* @param lowLim histograms with a value less than this will be listed as failed
* @param highLim histograms with a value higher than this will be listed as failed
* @param fileName name of a file to store the list of failed spectra in (pass "" to aviod writing to file)
* @return An array that of the index numbers of the histograms that fail
*/
std::vector<int> FindProblemDetectors::markDetects(
MatrixWorkspace_sptr responses, double lowLim, double highLim,
  std::string fileName)
{
  g_log.information("Apply the criteria to find failing detectors");
  //get dector information
  const SpectraDetectorMap& specMap = responses->spectraMap();
  Axis* specAxis = responses->getAxis(1);

  // get ready to write dead detectors to an array
  std::vector<int> badDets;
  // get ready to report the numbers found to the log
  int countSpec = 0, countLows = 0, countHighs = 0;
  // ready to write dead detectors to a file
  std::ofstream file;
  bool fileOpen = false;
  //it is not an error if the name is "", we'll just leave the file marked not open to ignore writing
  if ( !fileName.empty() )
  {
    file.open( fileName.c_str() );
    if ( file.rdstate() & std::ios::failbit )
    {
      g_log.error("Could not open file \"" + fileName + "\"");
    }
    //file opening ws successful, we'll write to the file
    else fileOpen = true;
  }

  if ( fileOpen ) file << "Index Spectrum UDET(S)" << std::endl;  

  // iterate over the data values setting the live and dead values
  g_log.information() << "Marking dead detectors" << std::endl;
  const int numSpec = m_MaxSpec - m_MinSpec;
  int iprogress_step = numSpec / 10;
  if (iprogress_step == 0) iprogress_step = 1;
  for (int i = 0; i <= numSpec; ++i)
  {
    double &y = responses->dataY(i)[0];
    if ( y > lowLim && y < highLim) y = GoodVal;
    else
    {
      std::string problem = "";
      if ( y < lowLim )
      {
        problem = "low";
        countLows++;
      }
      else
      {
        problem = "high";
        countHighs++;
      }
      y = BadVal;
      countSpec++;
            
      // Write the spectrum number to file
      const int specNo = specAxis->spectraNo(i);
      if ( fileOpen )
      {
        file << " Spectrum " << specNo << " is reading " << problem;
      }

      if ( fileOpen ) file << " detector IDs: "; 
      // Get the list of detectors for this spectrum and iterate over
      const std::vector<int> dets = specMap.getDetectors(specNo);
      std::vector<int>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it)
      {
        // don't put a comma before the first entry
        if ( fileOpen )
        {
          if ( it != dets.begin() ) file << ", ";
          file << " " << *it;
        }
        //write to the vector array that this function returns
        badDets.push_back(*it);
      }
      if ( fileOpen ) file << std::endl;
    }

    // update the progressbar information
    if (i % iprogress_step == 0)
    {
      advanceProgress( int(RTMarkDetects*i/float(numSpec)) );
      progress( m_PercentDone );
      interruption_point();
    }
  }
  //output and pass back what we found out about bad detectors
  if ( fileOpen ) file.close();
  g_log.information() << "Found a total of " << countLows <<
    " detectors reading low and " << countHighs << " reading high in " <<
    countSpec << " 'bad' spectra." << std::endl;
  //set the workspace to have no units
  responses->isDistribution(false);
  responses->setYUnit("");
  return badDets;
}
/// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
float FindProblemDetectors::advanceProgress(int toAdd)
{
  m_PercentDone += toAdd/float(m_TotalTime);
  // it could go negative as sometimes the percentage is re-estimated backwards, this is worrying about if a small negative value could cause an error
  m_PercentDone = std::abs(m_PercentDone);
  return m_PercentDone;
}
/// Update the percentage complete estimate assuming that the algorithm aborted a task with estimated RunTime toAdd
void FindProblemDetectors::failProgress(RunTime aborted)
{
  advanceProgress(-aborted);
  m_TotalTime -= aborted;
};

} // namespace Algorithm
} // namespace Mantid
