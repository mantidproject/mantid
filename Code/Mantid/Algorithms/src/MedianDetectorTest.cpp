#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
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
DECLARE_ALGORITHM(MedianDetectorTest)

using namespace Kernel;
using namespace API;

void MedianDetectorTest::init()
{
  HistogramValidator<MatrixWorkspace> *val =
    new HistogramValidator<MatrixWorkspace>;
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input,val),
    "Name of the input workspace" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Each histogram from the input workspace maps to a histogram in this\n"
    "workspace that has just one value which indicates if there was a\n"
    "bad detector" );

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0);
  declareProperty("SignificanceTest", 3.3, mustBePositive,
    "Set this to a nonzero value and detectors in spectra with a total\n"
    "number of counts is within this number of standard deviations from the\n"
    "median will not be labelled bad (default 3.3)" );
  declareProperty("LowThreshold", 0.1, mustBePositive->clone(),
    "Detectors corresponding to spectra with total counts less than this\n"
    "proportion of the median number of counts would be labelled as reading\n"
    "badly (default 0.1)" );
  declareProperty("HighThreshold", 1.5, mustBePositive->clone(),
    "Detectors corresponding to spectra with total counts more than this\n"
    "number of the median would be labelled as reading badly (default 1.5)" );
/* Allow users to change the failure codes?      declareProperty("LiveValue", 0.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'live'\n"
        "(default 0.0)");
      declareProperty("DeadValue", 100.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'dead'\n"
        "(default 100.0)" );*/
  BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
  mustBePosInt->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
    "The index number of the first spectrum to include in the calculation\n"
    "(default 0)" );
  //UNSETINT and EMPTY_DBL() are tags that indicate that no value has been set and we want to use the default
  declareProperty("EndWorkspaceIndex", UNSETINT, mustBePosInt->clone(),
    "The index number of the last spectrum to include in the calculation\n"
    "(default the last histogram)" );
  declareProperty("RangeLower", EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be included\n"
    "in the summation used to decide if a detector is 'bad' (default: the\n"
    "start of each histogram)" );
  declareProperty("RangeUpper", EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be included in the summation used to decide if a detector is 'bad'\n"
    "(default: the end of each histogram)" );
  declareProperty("OutputFile","",
    "The name of a file to write the list of dead detector UDETs (default\n"
    "no file output)" );
      // This output property will contain the list of UDETs for the dead detectors
  declareProperty("BadDetectorIDs",std::vector<int>(),Direction::Output);
}

/** Executes the algorithm that includes calls to SolidAngle and Integration
*
*  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*  @throw runtime_error if algorithm cannot execute
*/
void MedianDetectorTest::exec()
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
    //if some numbers in angles are zero we will get the infinty flag value in the output work space which needs to be dealt with later
    counts = counts/angles;     
  }
  //Gets an average of the data, the medain is less influenced by a small number of huge values than the mean
  double av = getMedian(counts);
  //The final piece of the calculation, remove any detectors whoses signals are outside the threshold range
  std::vector<int> outArray = FindDetects( counts, av );// function uses reads several properties, the ErrorThreshold, lower and upper thresholds and the outputfile

  // Now the calculation is complete, setting the output property to the workspace will register it in the Analysis Data Service and allow the user to see it
  setProperty("OutputWorkspace", counts);

  setProperty("BadDetectorIDs", outArray);
}
/** Loads and checks the values passed to the algorithm
*
*  @throw invalid_argument if there is an incapatible property value so the algorithm can't continue
*/
void MedianDetectorTest::retrieveProperties()
{
  m_InputWS = getProperty("InputWorkspace");
  int maxSpecIndex = m_InputWS->getNumberHistograms() - 1;
  // construting this object will throw an invalid_argument if there is no instrument information, we don't catch it we the algorithm will be stopped
  InputWSDetectorInfo testTesting(m_InputWS);
  try
  {//more testing
    testTesting.aDetecIsMaskedinSpec(0);
  }
  catch(Kernel::Exception::NotFoundError)
  {// we assume we are here because there is no masked detector map, 
    // disable future calls to functions that use the detector map
    m_usableMaskMap = false;
    // it still makes sense to carry on
    g_log.warning(
      "Precision warning: Detector masking map can't be found, assuming that no detectors have been previously marked unreliable in this workspace");
  }

  m_MinSpec = getProperty("StartWorkspaceIndex");
  if ( (m_MinSpec < 0) || (m_MinSpec > maxSpecIndex) )
  {
    g_log.warning("StartSpectrum out of range, changed to 0");
    m_MinSpec = 0;
  }
  m_MaxSpec = getProperty("EndWorkspaceIndex");
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
/// Calculates the sum of soild angles of detectors for each histogram
/** Makes a worksapce with the total solid angle all the detectors in each spectrum cover from the sample
*  note returns an empty shared pointer on failure, uses the SolidAngle algorithm
* @param input A pointer to a workspace
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return A pointer to the workspace (or an empty pointer)
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(
            API::MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information("Calculating soild angles");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetSolidAngle);
  IAlgorithm_sptr childAlg = createSubAlgorithm("SolidAngle", t0, t1);
  childAlg->setProperty( "InputWorkspace", input );
  childAlg->setProperty( "StartWorkspaceIndex", firstSpec );
  childAlg->setProperty( "EndWorkspaceIndex", lastSpec );
  try
  {
  // Execute the sub-algorithm, it could throw a runtime_error at this point which would abort execution
    childAlg->execute();
    if ( ! childAlg->isExecuted() )
    {
      throw std::runtime_error("Unexpected problem calculating solid angles");
    }
  }
  //catch all exceptions because the solid angle calculation is optional
  catch(std::exception e)
  {
    g_log.warning(
      "Precision warning:  Can't find detector geometry " + name() +
      " will continue with the solid angles of all spectra set to the same value" );
    failProgress(RTGetSolidAngle);
    //The return is an empty workspace pointer, which must be handled by the calling function
    MatrixWorkspace_sptr empty;
    //function returns normally
    return empty;
  }
  return childAlg->getProperty("OutputWorkspace");
}

/// Calculates the sum counts in each histogram
/** Runs Integration as a sub-algorithm to get the sum of counts in the 
* range specfied by the algorithm properties Range_lower and Range_upper
* @param input points to the workspace to modify
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return Each histogram in the workspace has a single bin containing the sum of the bins in the input workspace
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getTotalCounts(
                API::MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information() << "Integrating input workspace" << std::endl;
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );
  
  childAlg->setProperty<MatrixWorkspace_sptr>( "InputWorkspace", input );
  childAlg->setProperty( "StartWorkspaceIndex", firstSpec );
  childAlg->setProperty( "EndWorkspaceIndex", lastSpec );
  // pass inputed values straight to this integration, checking must be done there
  childAlg->setPropertyValue( "RangeLower",  getPropertyValue("RangeLower") );
  childAlg->setPropertyValue( "RangeUpper", getPropertyValue("RangeUpper") );
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
/// Converts numbers of particle counts into count rates
/** Divides number of counts by time using ConvertToDistribution as a sub-algorithm
*
* @param counts A histogram workspace with counts in time bins 
* @return A workspace of the counts per unit time in each bin
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getRate
                                           (API::MatrixWorkspace_sptr counts)
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
/// Finds the median of numbers of counts in single bin histograms
/** Finds the median of values in single bin histograms rejecting spectra from masked
*  detectors and the results of divide by zero (infinite and NaN).  The median is an
*  average that is less affected by small numbers of very large values.
* @param input A histogram workspace with one entry in each bin
* @return The median value of the histograms in the workspace that was passed to it
* @throw logic_error if an input value is negative
*/
double MedianDetectorTest::getMedian(API::MatrixWorkspace_const_sptr input)
  const
{
  g_log.information("Calculating the median count rate of the spectra");

  // we need to check and exclude masked detectors
  InputWSDetectorInfo DetectorInfoHelper(input);
  // we'll allow a detector that has zero solid angle extent to the sample if it has no counts, I don't know if this neccessary but such unused detectors wont change the results of the calculation so why abort?  We just count them, warn people and mask people
  int numUnusedDects = 0;

  // make an array for all the values in the single bin histograms that can be converted to a C-style array for the GNU Scientifc Library
  MantidVec nums;
  // copy the data into this array
  for (int i = 0; i < input->getNumberHistograms(); ++i)
  {
    if ( (!m_usableMaskMap) || (!DetectorInfoHelper.aDetecIsMaskedinSpec(i)) )
    {
      double toCopy = input->readY(i)[0];
      //we shouldn't have negative numbers of counts, probably a SolidAngle correction problem
      if ( toCopy  < 0 )
      {
        g_log.debug() <<
          "Negative count rate found for spectrum number " <<
          DetectorInfoHelper.getSpecNum(i) << std::endl;
        throw std::logic_error(
          "Negative number of counts found, could be corrupted raw counts or solid angle data");
      }
      //there has been a divide by zero, likely to be due to a detector with zero solid angle
      if ( std::abs(toCopy) == std::numeric_limits<double>::infinity() )
      {
        g_log.debug() <<
          "numeric_limits<double>::infinity() found spectrum number "
          << DetectorInfoHelper.getSpecNum(i) << std::endl;
        throw std::runtime_error("Divide by zero error, one or more detectors has zero solid angle and a non-zero number of counts");
      }
      if ( toCopy != toCopy )
      {//this fun thing can happen if there was a zero divide by zero, solid angles again, as this, maybe, could be caused by a detector that is not used I wont exit because of this
        DetectorInfoHelper.maskAllDetectorsInSpec(i);
        // we'll report how many times this happened so that the user can think about how good, bad, there data is 
        numUnusedDects ++;
      }
      //if we get to here we have a good value, copy it over!
      nums.push_back( toCopy );
    }
  }
  if (numUnusedDects > 0)
  {
    g_log.debug() <<
      "Found \"Not a Number\" in the numbers of counts, assuming a least one detector with zero solid angle and zero counts was found"
       << std::endl;
    g_log.warning() << numUnusedDects <<
      " detectors were found with zero solid angle and no counts they have been masked and will be ignored"
      << std::endl;
  }
  //we need a sorted array to calculate the median
  gsl_sort( &nums[0], 1, nums.size() );//The address of foo[0] will return a pointer to a contiguous memory block that contains the values of foo. Vectors are guaranteed to store there memory elements in sequential order, so this operation is legal, and commonly used (http://bytes.com/groups/cpp/453169-dynamic-arrays-convert-vector-array)
  return gsl_stats_median_from_sorted_data( &nums[0], 1, nums.size() );
}
/// Produces a workspace of single value histograms that indicate if the spectrum is within limits
/** Takes a single valued histogram workspace and assesses which histograms are within the limits
*
* @param responses a workspace of histograms with one bin
* @param baseNum The number expected number of counts, spectra near to this number of counts won't fail
* @return An array that of the index numbers of the histograms that fail
*/
std::vector<int> MedianDetectorTest::FindDetects(
                           API::MatrixWorkspace_sptr responses, double baseNum)
{
  g_log.information("Apply the criteria to find failing detectors");
  
  // prepare to fail spectra with numbers of counts less than this
  double lowLim = baseNum*m_Low;
  // prepare to fail spectra with numbers of counts greater than this
  double highLim = baseNum*m_High;
  //but a spectra can't fail if the statistics show its value is consistent with the mean value, check the error and how many errorbars we are away
  double minNumStandDevs = getProperty("SignificanceTest");

  //an array that will store the IDs of bad detectors
  std::vector<int> badDets;
  // get ready to write to the log the number of bad detectors found
  int cLows = 0, cHighs = 0, cAlreadyMasked = 0;
  // ready to write dead detectors to a file
  std::ofstream file;
  bool fileOpen = false;
  std::string fileName = getProperty("OutputFile");
  //it is not an error if the name is "", we'll just leave the fileOpen == false to prevent writing
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

  // Main part of the function, iterate over the data values setting the live and dead values
  const int numSpec = m_MaxSpec - m_MinSpec;
  int iprogress_step = numSpec / 10;
  if (iprogress_step == 0) iprogress_step = 1;
  for (int i = 0; i <= numSpec; ++i)
  {
    // get the address of the value of the first bin the spectra, we'll check it's value and then write a pass or fail to that location
    double &yInputOutput = responses->dataY(i)[0];
    // hold information about whether it passes or fails and why
    std::string problem = "";
    // first look for detectors that have been marked as dead
    InputWSDetectorInfo DetectorInfoHelper(responses);
    if ( m_usableMaskMap && DetectorInfoHelper.aDetecIsMaskedinSpec(i) )
    {
       problem = ", detector already masked";
       cAlreadyMasked ++;
    }
    else // not already marked dead, check is the value within the acceptance range
    {
      if ( yInputOutput < lowLim )
      {// compare the difference against the size of the errorbar -statistical significance check
        if ( std::abs(yInputOutput-baseNum) > minNumStandDevs*responses->readE(i)[0] )
        {
          problem = "is too low";
          cLows++;
        }
      }
      if ( yInputOutput > highLim )
      {// check that the deviation is not within the errors
        if ( std::abs(yInputOutput-baseNum) > minNumStandDevs*responses->readE(i)[0] )
        {
          problem = "is too high";
          cHighs++;
        }
      }
    }
    if ( problem.empty() )
    {// it is an acceptable value; just write the good flag to the output workspace and go on to check the next value
        yInputOutput = GoodVal;
    }
    else
    {//we have a bad spectrum do the reporting
      //this is all that is need for the output workspace
      yInputOutput = BadVal;
      // Write the spectrum number to file
      if ( fileOpen )
      {
        file << " Spectrum number " << DetectorInfoHelper.getSpecNum(i)
          << " " << problem;
      }
      
      if ( fileOpen ) file << ", detector IDs:"; 
      // Get the list of detectors for this spectrum and iterate over
      const std::vector<int> dets = DetectorInfoHelper.getDetectors(i);
      std::vector<int>::const_iterator it = dets.begin();
      for ( ; it != dets.end(); ++it)
      {
        //write to the vector array that this function returns
        badDets.push_back(*it);
        // now record to file
        if ( fileOpen )
        {
        // don't put a comma before the first entry
          if ( it != dets.begin() ) file << ", ";
          file << " " << *it;
        }
      }
      if ( fileOpen ) file << std::endl;
    }
    // the y values are just aribitary flags, an error value doesn't make sense now
    responses->dataE(i)[0] = 0;

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
  g_log.information() << "Found a total of " << cLows <<
    " spectra with low counts and " << cHighs << "spectra with high counts, "<<
    cAlreadyMasked << " were already marked bad." << std::endl;
  //set the workspace to have no units
  responses->isDistribution(false);
  responses->setYUnit("");
  return badDets;
}
/// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
float MedianDetectorTest::advanceProgress(int toAdd)
{
  m_PercentDone += toAdd/float(m_TotalTime);
  // it could go negative as sometimes the percentage is re-estimated backwards, this is worrying about if a small negative value could cause an error
  m_PercentDone = std::abs(m_PercentDone);
  return m_PercentDone;
}
/// Update the percentage complete estimate assuming that the algorithm aborted a task with estimated RunTime toAdd
void MedianDetectorTest::failProgress(RunTime aborted)
{
  advanceProgress(-aborted);
  m_TotalTime -= aborted;
};

} // namespace Algorithm
} // namespace Mantid
