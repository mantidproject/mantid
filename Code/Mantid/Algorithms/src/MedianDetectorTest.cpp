#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/FileProperty.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <cmath>
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
using namespace Geometry;

/// Default constructor
MedianDetectorTest::MedianDetectorTest() :
  API::Algorithm(), m_Low(0.1), m_High(1.5), m_MinSpec(0), m_MaxSpec(EMPTY_INT()),
  m_fracDone(0.0), m_TotalTime(RTTotal)
{
};

/// Declare algorithm properties
void MedianDetectorTest::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
    new HistogramValidator<>),
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
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt->clone(),
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
  declareProperty(new FileProperty("OutputFile","",FileProperty::OptionalSave),
    "The name of a file to write the list spectra that have a bad detector\n"
    "(default no file output)");
  declareProperty("GoodValue", 0.0,
    "For each input workspace spectrum that passes write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 0.0)");
  declareProperty("BadValue", 100.0,
    "For each input workspace spectrum that fails write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 100.0)" );
      // This output property will contain the list of UDETs for the dead detectors
  declareProperty("BadSpectraNums",std::vector<int>(),Direction::Output);
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
  //Adds the counts from all the bins and puts them in one total bin, calls subalgorithm Integration
  MatrixWorkspace_sptr counts = getTotalCounts(m_MinSpec, m_MaxSpec);//InputWorkspace and range properties are also read by this function
  //Divides the total number of counts by the number of seconds, calls the ConvertToDistribution algorithm, this isn't needed if all spectra have commonbins
  counts = getRate(counts); 
  //calls subalgorithm SolidAngle to get the total solid angle of the detectors that acquired each spectrum or nul on failure
  MatrixWorkspace_sptr angles = getSolidAngles(m_MinSpec, m_MaxSpec);// function uses the InputWorkspace property
  //Gets the count rate per solid angle (in steradians), if it exists, for each spectrum
  //this calculation is optional, it depends on angle information existing
  if ( angles.use_count() == 1 )
  {
    //if some numbers in angles are zero we will get the infinity flag value in the output work space which needs to be dealt with later
    counts = counts/angles;     
  }
  // An average of the data, the medain is less influenced by a small number of huge values than the mean
  double av = getMedian(counts);
  
  // The final piece of the calculation!
  const std::string outFile = getPropertyValue("outputfile");
  std::vector<int> deadList;
  // Find report and mask any detectors whoses signals are outside the threshold range
  FindDetects(counts, av, deadList, outFile);

  // Now the calculation is complete, setting the output property to the workspace will register it in the Analysis Data Service and allow the user to see it
  setProperty("OutputWorkspace", counts);

  setProperty("BadSpectraNums", deadList);
}

/** Loads and checks the values passed to the algorithm
*
*  @throw invalid_argument if there is an incapatible property value so the algorithm can't continue
*/
void MedianDetectorTest::retrieveProperties()
{
  m_InputWS = getProperty("InputWorkspace");
  int maxSpecIndex = m_InputWS->getNumberHistograms() - 1;

  m_MinSpec = getProperty("StartWorkspaceIndex");
  if ( (m_MinSpec < 0) || (m_MinSpec > maxSpecIndex) )
  {
    g_log.warning("StartSpectrum out of range, changed to 0");
    m_MinSpec = 0;
  }
  m_MaxSpec = getProperty("EndWorkspaceIndex");
  if (m_MaxSpec == EMPTY_INT() ) m_MaxSpec = maxSpecIndex;
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

/** Makes a worksapce with the total solid angle all the detectors in each spectrum cover from the sample
*  note returns an empty shared pointer on failure, uses the SolidAngle algorithm
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return A pointer to the workspace (or an empty pointer)
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(int firstSpec, int lastSpec )
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


/** Runs Integration as a sub-algorithm to get the sum of counts in the 
* range specfied by the algorithm properties Range_lower and Range_upper
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return Each histogram in the workspace has a single bin containing the sum of the bins in the input workspace
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getTotalCounts(int firstSpec, int lastSpec)
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

/** Divides number of counts by time using ConvertToDistribution as a sub-algorithm
*
* @param counts A histogram workspace with counts in time bins 
* @return A workspace of the counts per unit time in each bin
* @throw invalid_argument if the ConvertToDistribution validation on the input workspace fails (a workspace that is already a distribution is acceptable)
* @throw runtime_error if there is an during the execution of ConvertToDistribution
*/
API::MatrixWorkspace_sptr MedianDetectorTest::getRate(API::MatrixWorkspace_sptr counts)
{
  g_log.information("Calculating time averaged count rates");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetRate);
  IAlgorithm_sptr childAlg = createSubAlgorithm("ConvertToDistribution", t0, t1);
  try
  {
    childAlg->setProperty<MatrixWorkspace_sptr>( "Workspace", counts ); 
  }
  catch (std::invalid_argument &e)
  {
    std::string prob = e.what();
    if ( prob.find("A workspace containing numbers of counts is required here")
      != std::string::npos)
    {// this error means that the input workspace was already a distribution and so we don't need to convert it but just continue
      g_log.debug("Could not convert Workspace to a distribution as the workspace seems to be a distribution already, continuing");
      return counts;
    }
    throw;
  }   
  try
  {
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

/** Finds the median of values in single bin histograms rejecting spectra from masked
*  detectors and the results of divide by zero (infinite and NaN).  The median is an
*  average that is less affected by small numbers of very large values.
* @param input A histogram workspace with one entry in each bin
* @return The median value of the histograms in the workspace that was passed to it
* @throw out_of_range if a value is incountered that is unbelievibly high or negative
*/
double MedianDetectorTest::getMedian(API::MatrixWorkspace_sptr input)
{
  g_log.information("Calculating the median count rate of the spectra");

  // we need to check and exclude masked detectors
  int numUnusedDects(0);
  std::vector<int> badIndices;

  std::vector<double> goodValues;
  const int nhists(input->getNumberHistograms());
  // reduce the number of memory alloctions because we know there is probably one number of each histogram
  goodValues.reserve(nhists);
  // copy the data into this array
  PARALLEL_FOR1(input)
  for (int i = 0; i < nhists; ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    IDetector_sptr det;
    try
    {
      det = input->getDetector(i);
    }
    catch (Exception::NotFoundError&)
    {
      badIndices.push_back(i);
      continue;
    }
    double toCopy = input->readY(i)[0];
    // We shouldn't have negative numbers of counts, probably a SolidAngle correction problem
    if ( toCopy  < 0 )
    {
      g_log.debug() << "Negative count rate found for spectrum index " << i << std::endl;
      throw std::out_of_range("Negative number of counts found, could be corrupted raw counts or solid angle data");
    }
    //there has been a divide by zero, likely to be due to a detector with zero solid angle
    if ( toCopy == std::numeric_limits<double>::infinity() )
    {
      PARALLEL_CRITICAL(MedianDetectorTest_mediana)
      {
	badIndices.push_back(i);
      }
      continue;
    }
    if ( toCopy != toCopy )
    {
      PARALLEL_CRITICAL(MedianDetectorTest_medianb)
      {
      ++numUnusedDects;
      }
      continue;
    }
    PARALLEL_CRITICAL(MedianDetectorTest_medianc)
    {
      //if we get to here we have a good value, copy it over!
      goodValues.push_back( toCopy );
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (numUnusedDects > 0)
  {
    g_log.debug() <<
      "Found \"Not a Number\" in the numbers of counts, assuming detectors with zero solid angle and zero counts were found\n";
    g_log.information() << numUnusedDects <<
      " spectra found with non-contributing detectors that have zero solid angle and no counts, they have not been used and the detectors have been masked in the input workspace\n";
  }

  if( !badIndices.empty() )
  {
    g_log.warning() << "There were " << badIndices.size() << " spectra whose contribution to the median was "
      "ignored due either to an infinite count rate or a missing detector definition. They have been masked.\n";
    maskBadSpectra(input, badIndices);
  }
	 
  //we need a sorted array to calculate the median
  std::sort(goodValues.begin(), goodValues.end());
  double median = gsl_stats_median_from_sorted_data( &goodValues[0], 1, goodValues.size() );
  g_log.notice() << name() <<
    ": The median integrated counts or soild angle normalised counts are " << median << std::endl;
  
  if ( median < 0 || median > DBL_MAX/10.0 )
  {// something has gone wrong
    throw std::out_of_range("The calculated value for the median was either negative or unreliably large");
  }
  return median;
}

/** Takes a single valued histogram workspace and assesses which histograms are within the limits
*
* @param responses a workspace of histograms with one bin
* @param baseNum The number expected number of counts, spectra near to this number of counts won't fail
* @param specNums must be empty(), the ID numbers of all the detectors that were found bad are inserted into this array
* @param filename write the list of spectra numbers for failed detectors to a file with this name
* @return An array that of the index numbers of the histograms that fail
*/
void MedianDetectorTest::FindDetects(API::MatrixWorkspace_sptr responses, const double baseNum, std::vector<int> &specNums, const std::string &filename)
{
  g_log.information("Applying the criteria to find failing detectors");
  
  // prepare to fail spectra with numbers of counts less than this
  double lowLim = baseNum*m_Low;
  // prepare to fail spectra with numbers of counts greater than this
  double highLim = baseNum*m_High;
  //but a spectra can't fail if the statistics show its value is consistent with the mean value, check the error and how many errorbars we are away
  double minNumStandDevs = getProperty("SignificanceTest");

  //an array that will store the spectra indexes related to bad detectors
  std::set<int> highs, missingDataIndices;
  // reuse an vector, just to save memory and CPU time
  std::set<int> lows;
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

  PARALLEL_FOR1(responses)
  for (int i = 0; i <= numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    // update the progressbar information
    if (i % progStep == 0)
    {
      progress(advanceProgress(progStep*static_cast<double>(RTMarkDetects)/numSpec));
    }

    IDetector_sptr det;
    try
    {
      det = responses->getDetector(i);
    }
    catch (Exception::NotFoundError&)
    {
      responses->dataY(i)[0] = BadVal;
      PARALLEL_CRITICAL(MedianDetectorTest_missa)
      {
	missingDataIndices.insert(i);
      }
      continue;
    }
    if ( det->isMasked() )
    {
      // first look for detectors that have been marked as dead
      PARALLEL_CRITICAL(MedianDetectorTest_missb)
      {
	cAlreadyMasked ++;
      }
      responses->dataY(i)[0] = BadVal;
      continue;
    }
    const double yIn = responses->dataY(i)[0];
    const double sig = minNumStandDevs*responses->readE(i)[0];
    if( std::abs(sig) == std::numeric_limits<double>::infinity() || sig!=sig )
    {
      PARALLEL_CRITICAL(MedianDetectorTest_missc)
      {
	lows.insert(responses->getAxis(1)->spectraNo(i));
      }
      continue;
    }
    if ( yIn <= lowLim )
    {
      // compare the difference against the size of the errorbar -statistical significance check
      if(baseNum - yIn > sig)
      {
	PARALLEL_CRITICAL(MedianDetectorTest_missd)
	{
	  lows.insert(responses->getAxis(1)->spectraNo(i));
	}
	responses->dataY(i)[0] = BadVal;
	continue;
      }
    }
    if (yIn >= highLim)
    {
      // compare the difference against the size of the errorbar -statistical significance check
      if(yIn - baseNum > sig)
      {
	PARALLEL_CRITICAL(MedianDetectorTest_misse)
	{
	  highs.insert(responses->getAxis(1)->spectraNo(i));
	}
	responses->dataY(i)[0] = BadVal;
	continue;
      }
    }
    // if we've got to here there were no problems
    responses->dataY(i)[0] = GoodVal;

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // a record is kept in the output file, however.
  writeFile(filename, lows, highs, missingDataIndices);
  logFinds(missingDataIndices.size(), lows.size(), highs.size(), cAlreadyMasked);

  specNums.clear();
  specNums.insert(specNums.end(), lows.begin(), lows.end());
  specNums.insert(specNums.end(), highs.begin(), highs.end());

}

/** Write a mask file which lists bad spectra in groups saying what the problem is. The file
* is human readable
*  @param fname name of file, if omitted no file is written
*  @param lowList list of spectra numbers for specttra that failed the test on low integral
*  @param highList list of spectra numbers for spectra with integrals that are too high
*  @param problemIndices spectrum indices for spectra that lack, this function tries to convert them to spectra numbers adn catches any IndexError exceptions
*/
void MedianDetectorTest::writeFile(const std::string &fname, const std::set<int> &lowList, 
				   const std::set<int> &highList, const std::set<int> &problemIndices)
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

  const int numEntries = lowList.size() + highList.size() + problemIndices.size();

  file << "---"<<name()<<"---" << std::endl;
  file << "----"<<"Low Integral : "<<lowList.size()<<"----" << std::endl;
  writeListToFile(file, lowList, numEntries);

  file << "----" << "High Integral : " << highList.size() << "----" << std::endl;
  writeListToFile(file, highList, numEntries);
  file << std::endl;
  file << "----" << "Spectra not linked to a valid detector in the instrument definition : " 
       << problemIndices.size() << "----" << std::endl;
  writeListToFile(file, problemIndices, numEntries, true);
  file << std::endl;

  file.close();
}

/**
 * Write a list of indices to a file
 * @param file The file stream
 * @param indices A set of indices to write
 * @param totalLines The total number of lines that will be written to the file
 * @param convertToSpectraNo If true, the index will be converted to a spectra number first
 */
void MedianDetectorTest::writeListToFile(std::ofstream & file, const std::set<int> & indices, const int totalLines,
					 bool convertToSpectraNo)
{
  double numLines = static_cast<double>(totalLines)/g_file_linesize;
  int progStep = static_cast<int>(ceil(numLines/30));
  int numWritten(0);
  for ( std::set<int>::const_iterator itr = indices.begin() ; itr != indices.end(); )
  {
    // output the spectra numbers of the failed spectra as the spectra number does change when a workspace is cropped
    if( convertToSpectraNo )
    {
      try
      {
	file << m_InputWS->getAxis(1)->spectraNo(*itr);
      }
      catch (Exception::IndexError&)
      {
	file << std::endl << "-Spectrum with index " << *itr << " does have a spectrum number";
      }
    }
    else
    {
      file << *itr;
    }
    ++numWritten;
    if ( ++itr == indices.end() || numWritten % g_file_linesize == 0 )
    {
      file << std::endl;
      if ( (numWritten-1) % progStep == 0 )
      {
        progress(advanceProgress( progStep*RTWriteFile/numLines) );
        progress( m_fracDone );
        interruption_point();
      }
    }
    else
    {
      file << " ";
    }
  }

}

/**
 * Log the findings of the algorithm
 * @param missing The number of missing detectors
 * @param low The number of spectra counting low
 * @param high The number of spectra counting high
 * @param alreadyMasked The number of spectra already masked when tested.
 */
void MedianDetectorTest::logFinds(size_t missing, size_t low, size_t high, int alreadyMasked)
{
  if ( missing > 0 )
  {
    g_log.warning() << "Detectors in " << missing << " spectra couldn't be masked because they were missing" << std::endl;
  }  
  g_log.information() << "Found " << low << " spectra with low counts and " << high << " spectra with high counts, "
		      << alreadyMasked << " were already marked bad." << std::endl;
}

/**
 * Mask a set of workspace indices given
 * @param inputWS The workspace to mask
 * @param badIndices A list of workspace indices to mask
 */
void MedianDetectorTest::maskBadSpectra(API::MatrixWorkspace_sptr inputWS, const std::vector<int> & badIndices)
{
  IAlgorithm_sptr masker = createSubAlgorithm("MaskDetectors");
  masker->setProperty<MatrixWorkspace_sptr>("Workspace", inputWS);
  masker->setProperty<std::vector<int> >("WorkspaceIndexList", badIndices);
  
  try
  {
    masker->execute();
  }
  catch(std::runtime_error&)
  {
    g_log.error() << "Error masking bad spectra found during median calculation\n.";
    throw;
  }
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
