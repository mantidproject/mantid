#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FileProperty.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <math.h>
#include <algorithm>
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
  declareProperty("LowThreshold", 0.1,
    "Detectors corresponding to spectra with total counts equal to or less\n"
    "than this proportion of the median number of counts will be identified\n"
    "as reading badly (default 0.1)" );
  declareProperty("HighThreshold", 1.5,
    "Detectors corresponding to spectra with total counts equal to or more\n"
    "than this number of the median will be identified as reading badly\n"
    "(default 1.5)" );
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
  declareProperty(new FileProperty("OutputFile","", FileProperty::Save),
    "The name of a file to write the list spectra that have a bad detector\n"
    "(default no file output)");
  declareProperty("GoodValue", 0.0,
    "For each input workspace spectrum that passes write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 0.0)");
  declareProperty("BadValue", 100.0,
    "For each input workspace spectrum that fails write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 100.0)" );
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
  MatrixWorkspace_sptr angles = getSolidAngles(m_MinSpec, m_MaxSpec);// function uses the InputWorkspace property
  //Adds the counts from all the bins and puts them in one total bin, calls subalgorithm Integration
  MatrixWorkspace_sptr counts = getTotalCounts(m_MinSpec, m_MaxSpec/*InputWorkspace and range properties are also read by this function*/);
  //Divides the total number of counts by the number of seconds, calls the ConvertToDistribution algorithm
  counts = getRate(counts);
  //Gets the count rate per solid angle (in steradians), if it exists, for each spectrum
  //this calculation is optional, it depends on angle information existing
  if ( angles.use_count() == 1 )
  {
    //if some numbers in angles are zero we will get the infinty flag value in the output work space which needs to be dealt with later
    counts = counts/angles;     
  }
  // An average of the data, the medain is less influenced by a small number of huge values than the mean
  double av = getMedian(counts);
  
  // The final piece of the calculation!
  const std::string outFile = getPropertyValue("outputfile");
  std::vector<int> deadList;
  // Find report and mask any detectors whoses signals are outside the threshold range
  FindDetects(counts,av,deadList, outFile);// function reads the properties ErrorThreshold, lower and upper thresholds

  // Now the calculation is complete, setting the output property to the workspace will register it in the Analysis Data Service and allow the user to see it
  setProperty("OutputWorkspace", counts);

  setProperty("BadDetectorIDs", deadList);
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
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return A pointer to the workspace (or an empty pointer)
*/
MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(int firstSpec, int lastSpec )
{
  g_log.information("Calculating soild angles");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetSolidAngle);
  IAlgorithm_sptr childAlg = createSubAlgorithm("SolidAngle", t0, t1);
  childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
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
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return Each histogram in the workspace has a single bin containing the sum of the bins in the input workspace
*/
MatrixWorkspace_sptr MedianDetectorTest::getTotalCounts(int firstSpec, int lastSpec )
{
  g_log.information() << "Integrating input workspace" << std::endl;
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );
  
  childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
  childAlg->setProperty( "StartWorkspaceIndex", firstSpec );
  childAlg->setProperty( "EndWorkspaceIndex", lastSpec );
  // pass inputed values straight to this integration trusting the checking done there
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
MatrixWorkspace_sptr MedianDetectorTest::getRate(MatrixWorkspace_sptr counts)
{
  g_log.information("Calculating time averaged count rates");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetRate);
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
* @throw out_of_range if an input value is infinity or negative
*/
double MedianDetectorTest::getMedian(MatrixWorkspace_const_sptr input) const
{
  g_log.information("Calculating the median count rate of the spectra");

  // we need to check and exclude masked detectors
  InputWSDetectorInfo DetectorInfoHelper(input);
  // we'll allow a detector that has zero solid angle extent to the sample if it has no counts, I don't know if this neccessary but such unused detectors wont change the results of the calculation so why abort?  We just count them, warn people and mask people
  int numUnusedDects = 0;
  int numUnfoundDects = 0;

  // make an array for all the values in the single bin histograms that can be converted to a C-style array for the GNU Scientifc Library
  MantidVec nums;
  // reduce the number of memory alloctions because we know there is probably one number of each histogram
  nums.reserve(input->getNumberHistograms());
  // copy the data into this array
  for (int i = 0; i < input->getNumberHistograms(); ++i)
  {
    try
    {
      if ( ( ! m_usableMaskMap ) ||
        ( ! DetectorInfoHelper.aDetecIsMaskedinSpec(i) ) )
      {
        double toCopy = input->readY(i)[0];
        //we shouldn't have negative numbers of counts, probably a SolidAngle correction problem
        if ( toCopy  < 0 )
        {
          g_log.debug() <<
            "Negative count rate found for spectrum index " << i << std::endl;
          throw std::out_of_range(
            "Negative number of counts found, could be corrupted raw counts or solid angle data");
        }
       //there has been a divide by zero, likely to be due to a detector with zero solid angle
        if ( std::abs(toCopy) == std::numeric_limits<double>::infinity() )
        {
          g_log.debug() <<
            "numeric_limits<double>::infinity() found" << std::endl;
          g_log.warning() <<
            "Divide by zero error found in the spectrum with index " << i <<  ", the spectrum has a non-zero number of counts but its detectors have zero solid angle. Check your instrument definition file." << std::endl;
          try
          {
            DetectorInfoHelper.maskAllDetectorsInSpec(i);
            g_log.error() << "Its detectors have been masked" << std::endl;
          }
          catch (std::exception)
          {
            throw Exception::InstrumentDefinitionError("Couldn't mask detectors that are missing shape information");
          }
        }
        if ( toCopy != toCopy )
        {//this fun thing can happen if there was a zero divided by zero, solid angles again, as this, maybe, could be caused by a detector that is not used I wont exit because of this we'll record how many times this happened so that the user can think about how good or bad, their data is 
          numUnusedDects ++;
        }
        //if we get to here we have a good value, copy it over!
        nums.push_back( toCopy );
      }
    }
    catch (Exception::NotFoundError e)
    {// I believe that detectors missing from the workspace shouldn't cause a problem and as it occurs with most raw files that I have I wont alert the user
      numUnfoundDects ++;
    }
  }
  if (numUnusedDects > 0)
  {
    g_log.debug() <<
      "Found \"Not a Number\" in the numbers of counts, assuming detectors with zero solid angle and zero counts were found"
       << std::endl;
    g_log.information() << numUnusedDects <<
      " spectra found with non-contributing detectors that have zero solid angle and no counts, they have not been used and the detectors have been masked in the input workspace"
      << std::endl;
    void maskAllDetectorsInSpec(int SpecIndex);
  }
  if (numUnfoundDects > 0)
  {
    g_log.warning() << name() << " ignored the values in " << numUnfoundDects <<
      " spectra that mapped to detectors that can't be found in the instrument definition" << std::endl;
  }
  //we need a sorted array to calculate the median
  gsl_sort( &nums[0], 1, nums.size() );//The address of foo[0] will return a pointer to a contiguous memory block that contains the values of foo. Vectors are guaranteed to store there memory elements in sequential order, so this operation is legal, and commonly used (http://bytes.com/groups/cpp/453169-dynamic-arrays-convert-vector-array)
  double median = gsl_stats_median_from_sorted_data( &nums[0], 1, nums.size() );
  g_log.information() << name() <<
    ": The median integrated counts or soild angle normalised counts " << median << std::endl;
  return median;
}
/// Produces a workspace of single value histograms that indicate if the spectrum is within limits
/** Takes a single valued histogram workspace and assesses which histograms are within the limits
*
* @param responses a workspace of histograms with one bin
* @param baseNum The number expected number of counts, spectra near to this number of counts won't fail
* @param badDets the ID numbers of all the detectors that were found bad will be added to the end of this array
* @param filename write the list of spectra numbers for failed detectors to a file with this name
* @return An array that of the index numbers of the histograms that fail
*/
void MedianDetectorTest::FindDetects(MatrixWorkspace_sptr responses, const double baseNum, std::vector<int> &badDets, const std::string &filename)
{
  g_log.information("Applying the criteria to find failing detectors");
  
  // prepare to fail spectra with numbers of counts less than this
  double lowLim = baseNum*m_Low;
  // prepare to fail spectra with numbers of counts greater than this
  double highLim = baseNum*m_High;
  //but a spectra can't fail if the statistics show its value is consistent with the mean value, check the error and how many errorbars we are away
  double minNumStandDevs = getProperty("SignificanceTest");

  //an array that will store the spectra indexes related to bad detectors
  std::vector<int> lows, highs, missingDataIndices;
  // get ready to write to the log the number of bad detectors found
  int cAlreadyMasked = 0;

  // prepare to report progress
  const int numSpec = m_MaxSpec - m_MinSpec;
  const int progStep = static_cast<int>(ceil(numSpec/30.0));

  // we will go through the workspace checking the values and setting them to the flag values
  const double GoodVal = getProperty("GoodValue");
  const double BadVal = getProperty("BadValue");
  //set the workspace to have no units
  responses->isDistribution(false);
  responses->setYUnit("");

  // we use the functions in this class to check the detector masking
  const InputWSDetectorInfo DetectorInfoHelper(responses);
  for (int i = 0; i <= numSpec; ++i)
  {// update the progressbar information
    if (i % progStep == 0)
    {
      progress(advanceProgress( int(RTMarkDetects*i/float(numSpec)) ));
    }

    // get the address of the value of the first bin the spectra, we'll check it's value and then write a pass or fail to that location
    double &yInputOutput = responses->dataY(i)[0];
    try
    {
      if ( m_usableMaskMap && DetectorInfoHelper.aDetecIsMaskedinSpec(i) )
      {// first look for detectors that have been marked as dead
        cAlreadyMasked ++;
        yInputOutput = BadVal;
        continue;
      }
      if ( yInputOutput <= lowLim )
      {// compare the difference against the size of the errorbar -statistical significance check
        if ( std::abs(yInputOutput-baseNum) > minNumStandDevs*responses->readE(i)[0] )
        {
          lows.push_back(responses->getAxis(1)->spectraNo(i));
          yInputOutput = BadVal;
          continue;
        }
      }
      if ( yInputOutput >= highLim )
      {// check that the deviation is not within stastical error
        if ( std::abs(yInputOutput-baseNum) > minNumStandDevs*responses->readE(i)[0] )
        {
          highs.push_back(responses->getAxis(1)->spectraNo(i));
          yInputOutput = BadVal;
          continue;
        }
      }
      // if we've got to here there were no problems
      yInputOutput = GoodVal;
    }
    catch (Exception::NotFoundError e)
    {// I believe that detectors missing from the workspace shouldn't cause a problem and as it occurs with most raw files that I have I wont alert the user
      yInputOutput = BadVal;
      // adding entries to this array causes a log to written below
      missingDataIndices.push_back(i);
    }
  }

  // the output array doesn't list missingDataIndices because the array is used for masking detectors and informing users of the numbers of faulty instruments. A log is produced below at warning
  createOutputArray(lows, highs, responses->spectraMap(), badDets);
  // arecord is kept in the output file, however
  writeFile(filename, lows, highs, missingDataIndices);
  logFinds(missingDataIndices.size(), lows.size(), highs.size(), cAlreadyMasked);
}
/** Create an array of detector IDs from the two arrays of spectra numbers that were passed to it
*  @param lowList a list of spectra numbers
*  @param highList another list of spectra numbers
*  @param detMap the map that contains the list of detectors associated with each spectrum
*  @param total output property, the array of detector IDs
*/
void MedianDetectorTest::createOutputArray(const std::vector<int> &lowList, const std::vector<int> &highList, const SpectraDetectorMap& detMap, std::vector<int> &total) const
{
  // this assumes that each spectrum has only one detector, MERLIN has 4 if there are lots of dead detectors may be we should increse this
  total.reserve(lowList.size()+highList.size());

  for ( std::vector<int>::size_type i = 0; i < lowList.size(); ++i )
  {
    std::vector<int> tStore = detMap.getDetectors(lowList[i]);
    total.resize(total.size()+tStore.size());
    copy( tStore.begin(), tStore.end(), total.end()-tStore.size() );
  }

  for ( std::vector<int>::size_type i = 0; i < highList.size(); ++i )
  {
    std::vector<int> tStore = detMap.getDetectors(highList[i]);
    total.resize(total.size()+tStore.size());
    copy( tStore.begin(), tStore.end(), total.end()-tStore.size() );
  }
}
/** Write a mask file which lists bad spectra in groups saying what the problem is. The file
* is human readable
*  @param fname name of file, if omitted no file is written
*  @param lowList list of spectra numbers for specttra that failed the test on low integral
*  @param highList list of spectra numbers for spectra with integrals that are too high
*  @param problemIndices spectrum indices for spectra that lack, this function tries to convert them to spectra numbers adn catches any IndexError exceptions
*/
void MedianDetectorTest::writeFile(const std::string &fname, const std::vector<int> &lowList, const std::vector<int> &highList, const std::vector<int> &problemIndices) const
{
  //it's not an error if the name is "", we just don't write anything
  if ( fname.empty() )
  {
    return;
  }

  // open the output file for writing, blanking any existing content
  std::ofstream file( fname.c_str(), std::ios_base::out );
  if ( file.rdstate() & std::ios::failbit )
  {
    g_log.error("Could not open file \"" + fname + "\", file output disabled");
    return;
  }

  file << "---"<<name()<<"---" << std::endl;
  file << "----"<<"Low Integral : "<<lowList.size()<<"----" << std::endl;
  for ( std::vector<int>::size_type i = 0 ; i < lowList.size(); ++i )
  {// output the spectra numbers of the failed spectra as the spectra number does change when a workspace is cropped
    file << lowList[i];
    if ( (i + 1) % 10 == 0 || i == lowList.size()-1 )
    {// write an end of line after every 10 entries or when we have run out of entries
      file << std::endl;
    }
    else
    {
      file << " ";
    }
  }
  file << "----" << "High Integral : " << highList.size() << "----" << std::endl;
  for ( std::vector<int>::size_type i = 0 ; i < highList.size(); ++i )
  {// output the spectra numbers of the failed spectra as the spectra number does change when a workspace is cropped
    file << highList[i];
    if ( (i + 1) % 10 == 0 || i == highList.size()-1 )
    {// write an end of line after every 10 entries and when we have run out of entries
      file << std::endl;
    }
    else
    {
      file << " ";
    }
  }
  file << std::endl;
  file << "----" << "Spectra not linked to a valid detector in the instrument definition : " << problemIndices.size() << "----" << std::endl;
  for ( std::vector<int>::size_type i = 0 ; i < problemIndices.size(); ++i )
  {
    try
    {
      file << m_InputWS->getAxis(1)->spectraNo(problemIndices[i]);
    }
    catch (Exception::IndexError)
    {
      file << std::endl << "-Spectrum with index " << i << " does have a spectrum number";
    }
    if ( (i + 1) % 10 == 0 || i == problemIndices.size()-1 )
    {// write an end of line after every 10 entries and when we have run out of entries
      file << std::endl;
    }
    else
    {
      file << " ";
    }
  }
  file << std::endl;

  file.close();
}
/// called by FindDetects() to write a summary to g_log
void MedianDetectorTest::logFinds(std::vector<int>::size_type missing, std::vector<int>::size_type low, std::vector<int>::size_type high, int alreadyMasked)
{
  if ( missing > 0 )
  {
    g_log.warning() << "Detectors in " << missing << " spectra couldn't be masked because they were missing" << std::endl;
  }  
  g_log.information() << "Found " << low << " spectra with low counts and " << high << " spectra with high counts, "<< alreadyMasked << " were already marked bad." << std::endl;
}
/** Update the percentage complete estimate assuming that the algorithm has completed a task with the
* given estimated run time
* @param toAdd the estimated additional run time passed since the last update, where m_TotalTime holds the total algorithm run time
* @return estimated fraction of algorithm runtime that has passed so far
*/
double MedianDetectorTest::advanceProgress(double toAdd)
{
  m_fracDone += toAdd/m_TotalTime;
  // it could go negative as sometimes the percentage is re-estimated backwards, this is worrying about if a small negative value will cause a problem some where
  m_fracDone = std::abs(m_fracDone);
  interruption_point();
  return m_fracDone;
}
/** Update the percentage complete estimate assuming that the algorithm aborted a task with the given
*  estimated run time
*  @param aborted the amount of algorithm run time that was saved by aborting a part of the algorithm, where m_TotalTime holds the total algorithm run time
*/
void MedianDetectorTest::failProgress(RunTime aborted)
{
  advanceProgress(-aborted);
  m_TotalTime -= aborted;
};

} // namespace Algorithm
} // namespace Mantid
