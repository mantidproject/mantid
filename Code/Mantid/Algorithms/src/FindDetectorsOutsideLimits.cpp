#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include <fstream>
#include <cmath>

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsOutsideLimits)

using namespace Kernel;
using namespace API;

/// Initialisation method.
void FindDetectorsOutsideLimits::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "Name of the input workspace2D" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Each histogram from the input workspace maps to a histogram in this\n"
    "workspace with one value that indicates if there was a dead detector" );
  declareProperty("HighThreshold", 0.0,
    "Spectra whose total number of counts are equal to or above this value\n"
    "will be marked bad (default 0)" );
  declareProperty("LowThreshold", 0.0,
    "Spectra whose total number of counts are equal to or below this value\n"
    "will be marked bad (default 0)" );

  declareProperty("GoodValue",0.0,
    "The value to be assigned to spectra flagged as 'live' (default 0.0)" );
  declareProperty("BadValue",100.0,
    "The value to be assign to spectra flagged as 'bad' (default 100.0)" );
  declareProperty("RangeLower", EMPTY_DBL(),
    "No bin with a boundary at an x value less than this will be used\n"
    "in the summation that decides if a detector is 'bad' (default: the\n"
    "start of each histogram)" );
  declareProperty("RangeUpper", EMPTY_DBL(),
    "No bin with a boundary at an x value higher than this value will\n"
    "be used in the summation that decides if a detector is 'bad'\n"
    "(default: the end of each histogram)" );
  declareProperty(new FileProperty("OutputFile","", FileProperty::OptionalSave),
    "The name of a file to write the list spectra that have a bad detector\n"
    "(default no file output)");
    // This output property will contain the list of UDETs for the dead detectors
  declareProperty("BadSpectraNums",std::vector<int>(),Direction::Output);
}

/** Executes the algorithm
*
*  @throws runtime_error Thrown if the algorithm cannot execute
*  @throws invalid_argument is the LowThreshold property is greater than HighThreshold
*/
void FindDetectorsOutsideLimits::exec()
{
  double liveValue = getProperty("GoodValue");
  double deadValue = getProperty("BadValue");
  double lowThreshold = getProperty("LowThreshold");
  double highThreshold = getProperty("HighThreshold");
  if ( highThreshold <= lowThreshold )
  {
    g_log.error() << name() << ": Lower limit (" << lowThreshold <<
      ") >= the higher limit (" << highThreshold <<
      "), all detectors in the spectrum would be marked bad";
    throw std::invalid_argument("The high threshold must be higher than the low threshold");
  }

  // Get the integrated input workspace
  MatrixWorkspace_sptr integratedWorkspace = integrateWorkspace();
  //set the workspace to have no units
  integratedWorkspace->isDistribution(false);
  integratedWorkspace->setYUnit("");

  // store the IDs of the spectra and detectors that fail
  std::set<int> lows, highs;

  // iterate over the data values setting the live and dead values
  const int numSpec = integratedWorkspace->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numSpec / 100.0));
  
  PARALLEL_FOR1(integratedWorkspace)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    if (i % progStep == 0)
    {
      progress(static_cast<double>(i)/numSpec);
      interruption_point();
    }
    // the Y-values are the input and one of the outputs of this function, the E-values, error values, are unchanged but will be meaning less at the end of this algorithm
    const double yValue = integratedWorkspace->readY(i)[0];
    if ( yValue <= lowThreshold )
    {
      PARALLEL_CRITICAL(fdol_a)
      {
        lows.insert(integratedWorkspace->getAxis(1)->spectraNo(i));
      }
      integratedWorkspace->dataY(i)[0] = deadValue;
      continue;
    }
    if ( yValue >= highThreshold )
    {
     PARALLEL_CRITICAL(fdol_b)
      {
        highs.insert(integratedWorkspace->getAxis(1)->spectraNo(i));
      }
      integratedWorkspace->dataY(i)[0] = deadValue;
      continue;     
    }
    // if we've got to here there were no problems, flag this
    integratedWorkspace->dataY(i)[0] = liveValue;

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  writeFile(getPropertyValue("OutputFile"), lows, highs);

  g_log.information() << "Found a total of " << lows.size() + highs.size() << " failing "
    << "spectra (" << lows.size() << " reading low and " << highs.size()
    << " reading high)" << std::endl;
  
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", integratedWorkspace);
  
  std::vector<int> badSpectra(lows.size() + highs.size(), 0);
  badSpectra.assign(lows.begin(), lows.end());
  badSpectra.insert(badSpectra.end(), highs.begin(), highs.end());

  setProperty("BadSpectraNums", badSpectra);
}

/** Run Integration as a sub-algorithm
 *  @return The workspace resulting from the algorithm
 *  @throw runtime_error can be passed up from the sub-algorithm throws
 */
MatrixWorkspace_sptr FindDetectorsOutsideLimits::integrateWorkspace()
{
  g_log.debug() << "Integrating input workspace" << std::endl;

  API::IAlgorithm_sptr childAlg = createSubAlgorithm("Integration");
  // Now execute integration. Catch and log any error
  try
  {
    //pass inputed values straight to Integration, checking must be done there
    childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
    childAlg->setPropertyValue( "RangeLower",  getPropertyValue("RangeLower") );
    childAlg->setPropertyValue( "RangeUpper", getPropertyValue("RangeUpper") );
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run Integration sub-algorithm");
    throw;
  }
  
  if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run Integration sub-algorithm");

  return childAlg->getProperty("OutputWorkspace"); 
}

/** Write a mask file which lists bad spectra in groups saying what the problem is. The file
* is human readable
*  @param fname name of file, if omitted no file is written
*  @param lowList list of spectra numbers for specttra that failed the test on low integral
*  @param highList list of spectra numbers for spectra with integrals that are too high
*/
void FindDetectorsOutsideLimits::writeFile(const std::string &fname, const std::set<int> &lowList, const std::set<int> &highList) const
{
  //it's not an error if the name is "", we just don't write anything
  if ( fname.empty() )
  {
    return;
  }

  g_log.debug() << "Opening output file " << fname << std::endl;
  // open the output file for writing, blanking any existing content
  std::ofstream file( fname.c_str(), std::ios_base::out );
  if ( file.rdstate() & std::ios::failbit )
  {
    g_log.error("Could not open file \"" + fname + "\", file output disabled");
    return;
  }
  
  file << "---" << name() << "---" << std::endl;
  writeList(file, lowList, "----Low Integral : ", "----");
  writeList(file, highList, "----High Integral : ", "----");
  file << std::endl;

  file.close();
  g_log.debug() << "File output complete" << std::endl;
}

/**
 * Write a list to an output stream
 * @param outputStream The stream to use to write out
 * @param specList The set of spectrum numbers to write out
 * @param prefix A string to prefix the number of spectra line
 * @param suffix A string to suffix the number of spectra line
 */
void FindDetectorsOutsideLimits::writeList(std::ostream & outputStream, const std::set<int> & specList, 
                                           const std::string & prefix, const std::string & suffix) const
{
  std::set<int>::size_type nentries = specList.size();
  outputStream << prefix << nentries << suffix << std::endl;
  const unsigned int blocksize(10);
  unsigned int item_count(0);
  const std::set<int>::const_iterator iend = specList.end();
  for ( std::set<int>::const_iterator itr = specList.begin() ; itr != iend; )
  {
    outputStream << *itr;
    ++item_count;
    if( ++itr != iend && item_count < blocksize)
    {
      outputStream << " "; 
    }
    else
    {
      item_count = 0;
      outputStream << std::endl;
    }
   }
}

}
}
