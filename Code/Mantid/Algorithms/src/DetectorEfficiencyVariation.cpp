//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorEfficiencyVariation.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include <boost/shared_ptr.hpp>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <cmath>
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

  BoundedValidator<double> *moreThanZero = new BoundedValidator<double>();
  // Variation can't be zero as we take its reciprocal, so I've set the minimum to something below which double precession arithmetic might start to fail
  moreThanZero->setLower(1e-280);
  declareProperty("Variation", 1.1, moreThanZero,
    "Identify spectra whose total number of counts has changed by more\n"
    "than this factor of the median change between the two input workspaces" );
  BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
  mustBePosInt->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
    "The index number of the first entry in the Workspace to include in\n"
    "the calculation (default: 0)" );

  //Mantid::EMPTY_INT() and EMPTY_DBL() are tags that indicate that no value has been set and we want to use the default
  declareProperty("EndWorkspaceIndex", Mantid::EMPTY_INT(), mustBePosInt->clone(),
    "The index number of the last entry in the Workspace to include in\n"
    "the calculation (default: the last spectrum in the workspace)" );
  declareProperty("RangeLower", Mantid::EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be included\n"
    "in the summation used to decide if a detector is 'bad' (default: the\n"
    "start of each histogram)" );
  declareProperty("RangeUpper", Mantid::EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be included in the summation used to decide if a detector is 'bad'\n"
    "(default: the end of each histogram)" );
  declareProperty(new FileProperty("OutputFile","",FileProperty::OptionalSave),
    "The name of a file to write the list spectra that have a bad detector\n"
    "(default no file output)");
      // This output property will contain the list of UDETs for the dead detectors
  declareProperty("GoodValue", 0.0,
    "For each input workspace spectrum that passes write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 0.0)");
  declareProperty("BadValue", 100.0,
    "For each input workspace spectrum that fails write this flag value\n"
    "to the equivalent spectrum in the output workspace (default 100.0)" );
  declareProperty("BadSpectraNums",std::vector<int>(),Direction::Output);
}

/** Executes the algorithm that includes calls to SolidAngle and Integration
*
*  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
*  @throw runtime_error if a sub-algorithm cannot execute
*/
void DetectorEfficiencyVariation::exec()
{
  MatrixWorkspace_sptr WB1;
  MatrixWorkspace_sptr WB2;
  MatrixWorkspace_sptr frac;
  double vari = Mantid::EMPTY_DBL();
  int minSpec = 0;
  int maxSpec = Mantid::EMPTY_INT();
  // sets the values passed to it with those from the algorithm properties, only that need checking are passed. Throws an invalid_argument if we can't find a good value for a property
  retrieveProperties( WB1, WB2, vari, minSpec, maxSpec);

  /* *** now do the calculations ...  ***/
  // Adds the counts from all the bins and puts them in one total bin, calls subalgorithm Integration
  MatrixWorkspace_sptr counts1 = getTotalCounts( WB1, minSpec, maxSpec );// function uses the data in the *Range properties too
  MatrixWorkspace_sptr counts2 = getTotalCounts( WB2, minSpec, maxSpec );// function uses the data in the *Range properties too
  try{// some divide by zeros happen when there are bad detectors.  We accept any divide by zeros here and check later and mask detectors that are a problem
  frac = counts1/counts2;     
  /*Mismatches in the number*/ } catch (std::invalid_argument) {
  /*of spectra however are not*/   g_log.error() << "The two input workspaces must be from the same instrument";
  /*allowed*/                      throw;}
  // Gets an average of the data (median is less influenced by small numbers of huge values) and checks and rejects data from divide by zero
  double av = getMedian(frac);
  
  // information on bad spectra will be writen to counts1 by this function, it looks for spectra whose number of counts differ in the two workspaces by more than frac
  setProperty("BadSpectraNums",
    findBad( counts1, counts2, av, vari, getProperty("OutputFile") ) );

  // counts1 was overwriten by the last function, now register it with the Analysis Data Service so that users can see it
  setProperty("OutputWorkspace", counts1);
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
  API::MatrixWorkspace_sptr &whiteBeam1, API::MatrixWorkspace_sptr &whiteBeam2,
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
  
  minSpec = getProperty("StartWorkspaceIndex");
  if ( (minSpec < 0) || (minSpec > maxSpecIndex) )
  {
    g_log.warning("StartWorkspaceIndex out of range, changed to 0");
    minSpec = 0;
  }
  maxSpec = getProperty("EndWorkspaceIndex");
  if (maxSpec == Mantid::EMPTY_INT()) maxSpec = maxSpecIndex;
  if ( (maxSpec < 0) || (maxSpec > maxSpecIndex ) )
  {
    g_log.warning("EndWorkspaceIndex out of range, changed to max Workspace number");
    maxSpec = maxSpecIndex;
  }
  if ( (maxSpec < minSpec) )
  {
    g_log.warning("EndWorkspaceIndex can not be less than the StartWorkspaceIndex, changed to max Workspace number");
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
            API::MatrixWorkspace_sptr input, int firstSpec, int lastSpec )
{
  g_log.information() << "Integrating input workspace" << std::endl;
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );

  childAlg->setProperty( "InputWorkspace", input );
  childAlg->setProperty( "StartWorkspaceIndex", firstSpec );
  childAlg->setProperty( "EndWorkspaceIndex", lastSpec );
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
double DetectorEfficiencyVariation::getMedian(API::MatrixWorkspace_const_sptr input) const
{
  g_log.information() << "Calculating the median count rate of the spectra" << std::endl;

  // we need to check and exclude masked detectors
  InputWSDetectorInfo DetectorInfoHelper(input);
  // stores the number of divide by zeros so that the user knows if they have bad data
  int numInfinities = 0;
  // when we can't access a detector
  int numUnfoundDects = 0;

  // make an array of all the values in the single bin histograms for passing to the GNU Scientifc Library
  MantidVec nums;
  nums.reserve(input->getNumberHistograms());
  // copy the data into this array
  for (int i = 0; i < input->getNumberHistograms(); ++i)
  {
    try
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
          g_log.debug() << name() <<
            ": numeric_limits<double>::infinity() found spectrum number " << DetectorInfoHelper.getSpecNum(i) << std::endl;
          //  However, if much less than half the data is like this its effect will be negligeble so we'll just count and report
          numInfinities ++;
        }
        // if we get to here we have a good value, copy it over!
        nums.push_back( toCopy );
      }
    }
    catch (Exception::NotFoundError e)
    {// I believe that detectors missing from the workspace shouldn't cause a problem and as it occurs with most raw files that I have I wont alert the user
      numUnfoundDects ++;
    }
  }
  if (numInfinities > 0)
  {
    g_log.error() << name() << ": In comparing workspaces " << numInfinities <<
      " zeros or divide by zeros were seen (out of " << input->getNumberHistograms() <<
      ")" << " these spectra wont be included" << std::endl;
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
    ": The median ratio of the spectra in the input workspaces is " << median << std::endl;
  return median;
}
/// Overwrites the first workspace with bad spectrum information, also outputs an array and a file
/** Overwrites the first workspace with bad spectrum information, bad detector information is writen
* to the array that is returned and both things are writen to a file
*
* @param a MUST have the same number of histograms as b, this single bin histogram input workspace is overwriten
* @param b MUST have the same number of histograms as a, this single bin histogram workspace will be compared to a
* @param average The median value of the ratio of the total number of counts between equivalent spectra in the two workspaces
* @param variation The ratio between equivalent spectra can be greater than the median ratio by this factor, if the variation is greater the detector will be marked bad
* @param fileName name of a file to store the list of failed spectra in (pass "" to aviod writing to file)
* @return An array that of the index numbers of the histograms that fail
*/
std::vector<int> DetectorEfficiencyVariation::findBad(
  API::MatrixWorkspace_sptr a, API::MatrixWorkspace_const_sptr b,
  const double average, double variation, const std::string &fileName)
{
  g_log.information("Apply the criteria to find failing detectors");

  // This algorithm will assume that r is more than 1
  if ( variation < 1 )
  {// DIAG in libISIS did this.  A variation of less than 1 doesn't make sense in this algorithm
    variation = 1/variation;
  }
  // criterion for if the the first spectrum is larger than expected
  double largest = average*variation;
  // criterion for if the the first spectrum is lower than expected
  double lowest = average/variation;

  // get ready to report the number of bad detectors found to the log
  int cAlreadyMasked = 0;
  // arrays that will store information about which spectra were found bad
  std::vector<int> badDets, badSpecs, missingDataIndices;

  // iterate over the data values setting the live and dead values
  // this relies on a and b having the same number of bins
  const int numSpec = a->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numSpec/30.0));

  const InputWSDetectorInfo Detector1Info(a);
  const InputWSDetectorInfo Detector2Info(b);

  double GoodVal = getProperty("GoodValue");
  double BadVal = getProperty("BadValue");
  for (int i = 0; i < numSpec; ++i)
  {
    // move progress bar
    if (i % progStep == 0)
    {
      advanceProgress( progStep*static_cast<double>(RTMarkDetects)/numSpec );
      progress( m_fracDone );
      interruption_point();
    }

    // first look for detectors that have been marked as dead
    try
    {
      if ( m_usableMaskMap &&
        ( Detector1Info.aDetecIsMaskedinSpec(i) ||
        Detector2Info.aDetecIsMaskedinSpec(i) ) )
      {
        cAlreadyMasked ++;
        a->dataY(i)[0] = BadVal;
        // move on to the next spectrum
        continue;
      }
      // not already marked bad, check is the value within the acceptance range
      // examine the data, which should all be in the first bin of each histogram
      double ratio = a->readY(i)[0]/b->readY(i)[0];
      if ( ( ratio > largest ) || ( ratio < lowest ) ) 
      {// either v1 or v2 is too big
        a->dataY(i)[0] = BadVal;
        badSpecs.push_back(a->getAxis(1)->spectraNo(i));
        // move on to the next spectrum
        continue;
      }
      // if we've got to here there were no problems
      a->dataY(i)[0] = GoodVal;
    }
    catch (Exception::NotFoundError e)
    {// I believe that detectors missing from the workspace shouldn't cause a problem and as it occurs with most raw files that I have I wont alert the user
      a->dataY(i)[0] = BadVal;
      // adding entries to this array causes a log to written below
      missingDataIndices.push_back(i);
    }
  }

  // a record is kept in the output file, however
  writeFile(fileName, badSpecs, missingDataIndices, a->getAxis(1));

  g_log.information() << "Found a total of " << badSpecs.size() <<
    " spectra that changed by more than " << 100.0*(variation-1.0) << "% over or "
    "under the median change. " <<
    cAlreadyMasked << " were already marked bad." << std::endl;
  //finish off setting up the output workspace, it should have no units
  a->isDistribution(false);
  a->setYUnit("");
  return badSpecs;
}
/** Write a mask file which lists bad spectra in groups saying what the problem is. The file
* is human readable
*  @param fname name of file, if omitted no file is written
*  @param badList the list of spectra numbers for spectra that failed the test
*  @param problemIndices spectrum indices for spectra that lack, this function tries to convert them to spectra numbers adn catches any IndexError exceptions
*  @param SpecNums contains the spectra numbers for all spectra indices
*/
void DetectorEfficiencyVariation::writeFile(const std::string &fname, const std::vector<int> &badList, const std::vector<int> &problemIndices, const Axis * const SpecNums)
{
  (void) SpecNums; //Avoid compiler warning
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

  // for the progress bar
  const double numLines = static_cast<double>(badList.size())/LINESIZE;
  const int progStep = LINESIZE*static_cast<int>(ceil(numLines/30));


  file << "---"<<name()<<"---" << std::endl;
  for ( std::vector<int>::size_type i = 0 ; i < badList.size(); ++i )
  {// output the spectra numbers of the failed spectra as the spectra number does change when a workspace is cropped
    file << badList[i];
    if ( (i + 1) % LINESIZE == 0 )
    {// write an end of line after every 10 entries
      file << std::endl;
      if ( (i + 1) % progStep == 0 )
      {
        progress(advanceProgress( progStep*RTWriteFile/numLines ));
        progress(m_fracDone);
        interruption_point();
      }
    }
    else
    {
      file << " ";
    }
  }
  file << std::endl;
  file << "----" << "Spectra not linked to a valid detector in the instrument definition : " << problemIndices.size() << "----" << std::endl;
  for ( std::vector<int>::size_type j = 0 ; j < problemIndices.size(); ++j )
  {
    try
    {
//      file << SpecNums->spectraNo(problemIndices[j]);
    }
    catch (Exception::IndexError)
    {
      file << std::endl << "-Spectrum with index " << j << " doesn't have a spectrum number";
    }
    if ( (j + 1) % LINESIZE == 0 )
    {// write an end of line after every 10 entries
      file << std::endl;
      if ( (j + 1) % progStep == 0 )
      {
        progress(advanceProgress( progStep*RTWriteFile/numLines) );
        progress(m_fracDone);
        interruption_point();
      }
    }
    else
    {
      file << " ";
    }
  }
  file << std::endl;

  file.close();
}

/// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
double DetectorEfficiencyVariation::advanceProgress(double toAdd)
{
  m_fracDone += toAdd/m_TotalTime;
  // it could go negative as sometimes the percentage is re-estimated backwards, this is worrying about if a small negative value could cause an error
  m_fracDone = std::abs(m_fracDone);
  return m_fracDone;
}

} // namespace Algorithm
} // namespace Mantid
