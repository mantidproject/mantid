//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorEfficiencyVariation.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <boost/shared_ptr.hpp>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DetectorEfficiencyVariation)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

void DetectorEfficiencyVariation::init()
{
  HistogramValidator<MatrixWorkspace> *val =
    new HistogramValidator<MatrixWorkspace>;
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("WhiteBeamBase", "",
    Direction::Input,val),
    "Name of a white beam vanadium workspace" );
  // The histograms, the detectors in each histogram and their first and last bin boundary must match
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("WhiteBeamCompare","",Direction::Input,
    val->clone()),
    "Name of a matching second white beam vanadium run from the same\n"
    "instrument" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Each histogram from the input workspace maps to a histogram in this\n"
    "workspace with one value that indicates if there was a dead detector" );

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  // it doesn't make sense for this to be less than one but I think it is OK if it has a high value
  mustBePositive->setLower(0);
  //UNSETINT and EMPTY_DBL() are tags that indicate that no value has been set and we want to use the default
  declareProperty("Variation", -EMPTY_DBL(), mustBePositive,
    "When the number of counts varies between input workspaces by more\n"
    "than this fraction of the median that histogram and its\n"
    "associated detectors are marked bad" );

  BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
  mustBePosInt->setLower(0);
  declareProperty("StartSpectrum", 0, mustBePosInt,
    "The index number of the first spectrum to include in the calculation\n"
    "(default: 0)" );
  declareProperty("EndSpectrum", UNSETINT, mustBePosInt->clone(),
    "The index number of the last spectrum to include in the calculation\n"
    "(default: the last spectrum in the workspace)" );
  declareProperty("RangeLower", EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be included\n"
    "in the summation used to decide if a detector is 'bad' (default: the\n"
    "start of each histogram)" );
  declareProperty("RangeUpper", EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be included in the summation used to decide if a detector is 'bad'\n"
    "(default: the end of each histogram)" );
  declareProperty("OutputFile","",
    "The name of a file to write the list of dead detector UDETs (default:\n"
    "no file output)" );
      // This output property will contain the list of UDETs for the dead detectors
  declareProperty("BadIDs",std::vector<int>(),Direction::Output);
}

/** Executes the algorithm that includes calls to SolidAngle and Integration
*
*  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*  @throw runtime_error if algorithm cannot execute
*/
void DetectorEfficiencyVariation::exec()
{
  MatrixWorkspace_sptr WB1;
  MatrixWorkspace_sptr WB2;
  double vari = EMPTY_DBL();
  int minSpec = 0;
  int maxSpec = UNSETINT;

  // sets the values passed to it with those from the algorithm properties, only that need checking are passed. Throws an invalid_argument if we can't find a good value for a property
  retrieveProperties( WB1, WB2, vari, minSpec, maxSpec);
  // now do the calculations ...  
  // Adds the counts from all the bins and puts them in one total bin, calls subalgorithm Integration
  MatrixWorkspace_sptr counts1 = getTotalCounts( WB1, minSpec, maxSpec );// function uses the data in the *Range properties too
  MatrixWorkspace_sptr counts2 = getTotalCounts( WB2, minSpec, maxSpec );// function uses the data in the *Range properties too
  // some divide by zeros could happen here but only if there are bad detectors and they should have been removed.  We accept any divide by zeros here and check later
  MatrixWorkspace_sptr frac = counts1/counts2;     
  
  // Gets an average of the data (median is less influenced by small numbers of huge values) and checks and rejects data from divide by zero
  double av = getMedian(frac);
  
  // information on bad spectra will be writen to counts1 by this function, it looks for spectra whose number of counts differ in the two workspaces by more than frac
  std::vector<int> outArray =
    markBad( counts1, counts2, av, vari, getProperty("OutputFile") );
  // counts1 was overwriten by the last function, now register it with the Analysis Data Service so that users can see it
  setProperty("OutputWorkspace", counts1);
  // make the output array visible to the user too
  setProperty("BadIDs", outArray);
}
/** Loads, checks and passes back the values passed to the algorithm
* @param whiteBeam1 A white beam vanadium spectrum that will be used to check detector efficiency variations
* @param whiteBeam2 The other white beam vanadium spectrum from the same instrument to use for comparison
* @param vari The maximum fractional variation above the median that is allowed for god detectors
* @param minSpec Index number of the first spectrum to use
* @param maxSpec Index number of the last spectrum to use
* @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*/
void DetectorEfficiencyVariation::retrieveProperties(
  MatrixWorkspace_sptr &whiteBeam1, MatrixWorkspace_sptr &whiteBeam2,
  double &vari, int &minSpec, int &maxSpec )
{
  whiteBeam1 = getProperty("WhiteBeamBase");
  whiteBeam2 = getProperty("WhiteBeamCompare");
  if ( whiteBeam1->getBaseInstrument()->getName() !=
    whiteBeam2->getBaseInstrument()->getName() )
  {
    throw std::invalid_argument("The two input white beam vanadium workspaces must be from the same instrument");
  }
  int maxSpecIndex = whiteBeam1->getNumberHistograms() - 1;
  if ( maxSpecIndex != whiteBeam2->getNumberHistograms() - 1 )
  {//we would get a crash later on if this were not true
    throw std::invalid_argument("The input white beam vanadium workspaces must be have the same number of histograms");
  }
  // construting this object will throw an invalid_argument if there is no instrument information, we don't catch it we the algorithm will be stopped
  InputWSDetectorInfo testingTesting(whiteBeam1);
  InputWSDetectorInfo testingTestingTesting(whiteBeam2); 
  try
  {//now try to access the detector map, this is non-fatal
    testingTesting.aDetecIsMaskedinSpec(0);
    testingTestingTesting.aDetecIsMaskedinSpec(0);
  }
  catch(Kernel::Exception::NotFoundError)
  {// we assume we are here because there is no masked detector map, 
    //disable future calls to functions that use the detector map
    m_usableMaskMap = false;
    // it still makes sense to carry on
    g_log.warning(
      "Precision warning: Detector masking map can't be found, assuming that no detectors have been previously marked unreliable in this workspace");
  }

  vari = getProperty("Variation");
  
  minSpec = getProperty("StartSpectrum");
  if ( (minSpec < 0) || (minSpec > maxSpecIndex) )
  {
    g_log.warning("StartSpectrum out of range, changed to 0");
    minSpec = 0;
  }
  maxSpec = getProperty("EndSpectrum");
  if (maxSpec == UNSETINT) maxSpec = maxSpecIndex;
  if ( (maxSpec < 0) || (maxSpec > maxSpecIndex ) )
  {
    g_log.warning("EndSpectrum out of range, changed to max spectrum number");
    maxSpec = maxSpecIndex;
  }
  if ( (maxSpec < minSpec) )
  {
    g_log.warning("EndSpectrum can not be less than the StartSpectrum, changed to max spectrum number");
    maxSpec = maxSpecIndex;
  }
}
/// Calculates the sum counts in each histogram
/** Runs Integration as a sub-algorithm to get the sum of counts in the 
* range specfied by the algorithm properties Range_lower and Range_upper
* @param input points to the workspace to modify
* @param firstSpec the index number of the first histogram to analyse
* @param lastSpec the index number of the last histogram to analyse
* @return Each histogram in the workspace has a single bin containing the sum of the bins in the input workspace
*/
MatrixWorkspace_sptr DetectorEfficiencyVariation::getTotalCounts(
                MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information() << "Integrating input workspace" << std::endl;
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_PercentDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );

  childAlg->setProperty( "InputWorkspace", input );
  childAlg->setProperty( "StartSpectrum", firstSpec );
  childAlg->setProperty( "EndSpectrum", lastSpec );
  // pass inputed values straight to this integration, checking must be done there
  childAlg->setPropertyValue( "RangeLower",  getPropertyValue("RangeLower") );
  childAlg->setPropertyValue( "RangeUpper", getPropertyValue("RangeUpper") );
  // try to execute this child algorithm, which is Integrate
  try
  {
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
/// Finds the median of numbers in single bin histograms
/** Finds the median of values in single bin histograms rejecting spectra from masked
*  detectors and the results of divide by zero (infinite and NaN).  The median is an
*  average that is less affected by small numbers of very large values.
* @param input A histogram workspace with one entry in each bin
* @return The median value of the histograms in the workspace that was passed to it
*/
double DetectorEfficiencyVariation::getMedian(MatrixWorkspace_const_sptr input) const
{
  g_log.information() << "Calculating the median count rate of the spectra" << std::endl;

  // we need to check and exclude masked detectors
  InputWSDetectorInfo DetectorInfoHelper(input);
  // stores the number of divide by zeros so that the user knows if they have bad data
  int numInfinities = 0;

  // make an array of all the values in the single bin histograms for passing to the GNU Scientifc Library
  MantidVec nums;
  // copy the data into this array
  for (int i = 0; i < input->getNumberHistograms(); ++i)
  {
    if ( (!m_usableMaskMap) || (!DetectorInfoHelper.aDetecIsMaskedinSpec(i)) )
    {
      double toCopy = input->readY(i)[0];
      // lots zeros and divide by zeros could be a sign of bad data and could affect our accurcy
      if ( (toCopy == 0) ||
        ( std::abs(toCopy) == std::numeric_limits<double>::infinity() ) ||
        // this fun thing can happen if there was a zero divide by zero
        ( toCopy != toCopy ) )
      {// we need to log the divide by zero because it could affect the results
        g_log.debug() <<
          "numeric_limits<double>::infinity() found spectrum number " << DetectorInfoHelper.getSpecNum(i) << std::endl;
        //Howver, if much less than half the data is like this its effect will be negligeble so we'll just count and report
        numInfinities ++;
      }
      // if we get to here we have a good value, copy it over!
      nums.push_back( toCopy );
    }
  }
  if (numInfinities > 0)
  {
    g_log.error() << numInfinities << " divide by zeros were seem in "
      << input->getNumberHistograms() << " histograms in the input workspaces."
      "  Consider running an algorithm like FindDeadDetectors on the input"
      " workspaces first" << std::endl;
  }
  //we need a sorted array to calculate the median
  gsl_sort( &nums[0], 1, nums.size() );//The address of foo[0] will return a pointer to a contiguous memory block that contains the values of foo. Vectors are guaranteed to store there memory elements in sequential order, so this operation is legal, and commonly used (http://bytes.com/groups/cpp/453169-dynamic-arrays-convert-vector-array)
  double median = gsl_stats_median_from_sorted_data( &nums[0], 1, nums.size() );
  g_log.information() <<
    "The median ratio of the spectra in the input workspaces is " << median << std::endl;
  return median;
}
/// Overwrites the first workspace with bad spectrum information, also outputs an array and a file
/** Overwrites the first workspace with bad spectrum information, bad detector information is writen
* to the array that is returned and both things are writen to a file
*
* @param a this single bin histogram input workspace is overwriten
* @param b single bin histogram input workspace that is compared to a
* @param average The median value of the ratio of the total number of counts between equivalent spectra in the two workspaces
* @param variation The ratio between equivalent spectra can be greater than the median value by this factor, if the variation is greater the detector will be marked bad
* @param fileName name of a file to store the list of failed spectra in (pass "" to aviod writing to file)
* @return An array that of the index numbers of the histograms that fail
*/
std::vector<int> DetectorEfficiencyVariation::markBad( MatrixWorkspace_sptr a,
  MatrixWorkspace_const_sptr b, double average, double variation,
  std::string fileName )
{
  g_log.information("Apply the criteria to find failing detectors");

  // criterion for if the the first spectrum is larger than expected
  double forwardLargest = average*(1+variation);
  // criterion for if the the first spectrum is lower than expected
  double forwardLowest = average*(1-variation);
  // these lines make the algorithm work identically if the workspaces are swapped
  double reverseLargest = (1+variation)/average;
  //because the user can enter the workspace either way around
  double reverseLowest = (1-variation)/average;

  // get ready to report the number of bad detectors found to the log
  int cChanged = 0, cAlreadyMasked = 0;
  //an array that will store the IDs of bad detectors
  std::vector<int> badDets;

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
  // this relies on a and b having the same number of bins
  const int numSpec = a->getNumberHistograms();
  int iprogress_step = numSpec / 10;
  if (iprogress_step == 0) iprogress_step = 1;
  for (int i = 0; i < numSpec; ++i)
  {
    // hold information about whether the histogram passes or fails and why
    std::ostringstream problem;
    // first look for detectors that have been marked as dead

    InputWSDetectorInfo Detector1Info(a);
    InputWSDetectorInfo Detector2Info(b);
    if ( m_usableMaskMap &&
      ( Detector1Info.aDetecIsMaskedinSpec(i) ||
        Detector2Info.aDetecIsMaskedinSpec(i) ) )
    {
      problem << "detector already masked";
      cAlreadyMasked ++;
    }
    else // not already marked bad, check is the value within the acceptance range
    {
      // examine the data, which should all be in the first bin of each histogram
      double v1 = a->readY(i)[0];
      double v2 = b->readY(i)[0];
      if ( ( v1/v2 > forwardLargest ) || ( v1/v2 < forwardLowest ) ||
        ( v2/v1 > reverseLargest ) || ( v2/v1 < reverseLowest ) ) 
      {// either v1 or v2 is too big, 
        problem << "the number of counts has changed by a factor of " <<
          std::setprecision(5) << v1/v2;
        cChanged++;
      }
    }
    if ( !problem.str().empty() )
    {//we have a bad spectrum, do the reporting
      // write to the output workput space, which is also an input workspace
      a->dataY(i)[0] = BadVal;
      // Write the spectrum number to file
      if ( fileOpen )
      {
        file << "In spectrum number " << Detector1Info.getSpecNum(i)
          << ", " << problem.str();
      }
      
      if ( fileOpen ) file << " detector IDs:"; 
      // Get the list of detectors for this spectrum and iterate over
      const std::vector<int> dets = Detector1Info.getDetectors(i);
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
    else// problem is empty
    {// this is a good spectrum, only need to write to the output workspace
      a->dataY(i)[0] = GoodVal;
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
  g_log.information() << "Marked a total of " << cChanged <<
    " spectra that changed by more than " << 100.0*variation << "% over or "
    "under the median change. " <<
    cAlreadyMasked << " were already marked bad." << std::endl;
  //finish off setting up the output workspace, it should have no units
  a->isDistribution(false);
  a->setYUnit("");
  return badDets;
}


/// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
float DetectorEfficiencyVariation::advanceProgress(int toAdd)
{
  m_PercentDone += toAdd/float(m_TotalTime);
  // it could go negative as sometimes the percentage is re-estimated backwards, this is worrying about if a small negative value could cause an error
  m_PercentDone = std::abs(m_PercentDone);
  return m_PercentDone;
}

} // namespace Algorithm
} // namespace Mantid
