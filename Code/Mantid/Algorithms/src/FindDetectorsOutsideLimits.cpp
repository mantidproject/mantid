#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <fstream>
#include <math.h>

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsOutsideLimits)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;

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

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0);
  declareProperty("GoodValue",0.0, mustBePositive,
    "The value to be assigned to spectra flagged as 'live' (default 0.0)" );
  declareProperty("BadValue",100.0, mustBePositive->clone(),
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
  declareProperty("BadDetectorIDs",std::vector<int>(),Direction::Output);
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

  g_log.debug() << "Finding dead detectors" << std::endl;
  // store the IDs of the spectra and detectors that fail
  std::vector<int> lows, highs, deadDets;

  // iterate over the data values setting the live and dead values
  const int numSpec = integratedWorkspace->getNumberHistograms();
  const int progStep = static_cast<int>(ceil(numSpec / 100.0));
  for (int i = 0; i < numSpec; ++i)
  {
    if (i % progStep == 0)
    {
      progress(static_cast<double>(i)/numSpec);
      interruption_point();
    }
    // the Y-values are the input and one of the outputs of this function, the E-values, error values, are unchanged but will be meaning less at the end of this algorithm
    double &yInputOutput = integratedWorkspace->dataY(i)[0];
    if ( yInputOutput <= lowThreshold )
    {
      lows.push_back(integratedWorkspace->getAxis(1)->spectraNo(i));
      yInputOutput = deadValue;
      continue;
    }
    if ( yInputOutput >= highThreshold )
    {
      highs.push_back(integratedWorkspace->getAxis(1)->spectraNo(i));
      yInputOutput = deadValue;
      continue;     
    }
    // if we've got to here there were no problems
    yInputOutput = liveValue;
  }

  createOutputArray(lows, highs, integratedWorkspace->spectraMap(), deadDets);
  writeFile(getPropertyValue("OutputFile"), lows, highs);

  g_log.information() << "Found a total of " << lows.size()+highs.size() << " failing "
    << "spectra (" << lows.size() << " reading low and " << highs.size()
    << " reading high)" << std::endl;
  
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", integratedWorkspace);
  setProperty("BadDetectorIDs", deadDets);
}

/** Run Integration as a sub-algorithm
* @throw runtime_error can be passed up from the sub-algorithm throws
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

/** Create an array of detector IDs from the two arrays of spectra numbers that were passed to it
*  @param lowList a list of spectra numbers
*  @param highList another list of spectra numbers
*  @param detMap the map that contains the list of detectors associated with each spectrum
*  @param total output property, the array of detector IDs
*/
void FindDetectorsOutsideLimits::createOutputArray(const std::vector<int> &lowList, const std::vector<int> &highList, const SpectraDetectorMap& detMap, std::vector<int> &total) const
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
*/
void FindDetectorsOutsideLimits::writeFile(const std::string &fname, const std::vector<int> &lowList, const std::vector<int> &highList) const
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

  file.close();
  g_log.debug() << "File output complete" << std::endl;
}

}
}
