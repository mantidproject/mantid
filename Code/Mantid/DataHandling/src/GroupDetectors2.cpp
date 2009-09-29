//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileProperty.h"
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "Poco/StringTokenizer.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <ios>
#include <set>
#include <vector>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors2)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_const_sptr;

/// (Empty) Constructor
GroupDetectors2::GroupDetectors2() : m_PercentDone(0) {}

/// Destructor
GroupDetectors2::~GroupDetectors2() {}

void GroupDetectors2::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,
    new CommonBinsValidator<Workspace2D>),"The name of the input 2D workspace");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the output workspace");
  declareProperty(new FileProperty("MapFile", "", FileProperty::NoExistLoad),
    "A file that contains lists of spectra to group. See the help for the\n"
    "file format");
  declareProperty(new ArrayProperty<int>("SpectraList"),
    "An array containing a list of the indexes of the spectra to combine\n"
    "(DetectorList and WorkspaceIndexList are ignored if this is set)" );
  declareProperty(new ArrayProperty<int>("DetectorList"), 
    "An array of detector ID's (WorkspaceIndexList is ignored if this is\n"
    "set)" );
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "An array of workspace indices to combine" );
  declareProperty("KeepUngroupedSpectra",false,
    "If true, ungrouped spectra will be kept in the output workspace");
}

void GroupDetectors2::exec()
{
  // Get the input workspace
  const Workspace2D_const_sptr inputWS = getProperty("InputWorkspace");
  const int numInHists = inputWS->getNumberHistograms();
  // Bin boundaries need to be the same, so do the full check on whether they actually are
  if (!API::WorkspaceHelpers::commonBoundaries(inputWS))
  {
    g_log.error() << "Can only group if the histograms have common bin boundaries" << std::endl;
    throw std::invalid_argument("Can only group if the histograms have common bin boundaries");
  }
  progress( m_PercentDone = CHECKBINS );
  interruption_point();
  
  // There may be alot of spectra so listing the the ones that aren't grouped could be big deal
  std::vector<int> unGroupedInds;
  unGroupedInds.reserve(numInHists);
  for( int i = 0; i < numInHists ; i++ )
  {
    unGroupedInds.push_back(i);
  }

  // stores lists of spectra indexes to group, although we never do an index search on it
  storage_map groupSpecInds;
  // read in the input parameters to make that map, if KeepUngroupedSpectra was set we'll need a list of the ungrouped spectrra too
  getList(inputWS, groupSpecInds, unGroupedInds);

  // converting the list into a set gets rid of repeated values, here the multiple GroupDetectors2::USED become one USED at the start
  const std::set<int> unGroupedSet(unGroupedInds.begin(), unGroupedInds.end());

  // Check what the user asked to be done with ungrouped spectra
  const bool keepAll = getProperty("KeepUngroupedSpectra");
  // ignore the one USED value in set or ignore all the ungrouped if the user doesn't want them
  const int numUnGrouped = keepAll ? unGroupedSet.size()-1 : 0;

  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,
    groupSpecInds.size()+ numUnGrouped, inputWS->readX(0).size(),
    inputWS->blocksize());

  // prepare to move the requested histograms into groups, first estimate how long for progress reporting
  double prog4Copy = ( (100 - m_PercentDone)/(numInHists-unGroupedSet.size()) )
    * keepAll ? static_cast<double>(numInHists-unGroupedSet.size())/numInHists: 1;
  
  // now do all the moving
  int outIndex = makeGroups(groupSpecInds, inputWS, outputWS, prog4Copy);
    
  // If we're keeping ungrouped spectra
  if (keepAll)
  {
    // copy them into the output workspace
    moveOthers(unGroupedSet, inputWS, outputWS, outIndex);
  }

  g_log.information() << name() << " algorithm has finished" << std::endl;

  setProperty("OutputWorkspace",outputWS);
}

/** Make a map containing spectra indexes to group, the indexes could have come from
*  file, or an array, spectra numbers ...
*  @param workspace the user selected input workspace
*  @param outputData spectra indexes to group
*  @param unUsedSpec spectra indexes that are members of any group
*/
void GroupDetectors2::getList(Workspace2D_const_sptr workspace,
                       storage_map &outputData, std::vector<int> &unUsedSpec)
{
  // There are several properties that may contain the user data go through them in order of precedence
  const std::string filename = getProperty("MapFile");
  if ( ! filename.empty() )
  {// The file property has been set so try to load the file
    try
    {
      // the format of the input file is in source file "GroupDetectors2.h"
      readTheFile(filename, workspace, outputData, unUsedSpec);
    }
    catch ( std::exception )
    {
      g_log.error() << name() << "Error reading input file " << filename << std::endl;
      throw;
    }
    return;
  }
  const std::vector<int> indexList = getProperty("WorkspaceIndexList");
  const std::vector<int> spectraList = getProperty("SpectraList");
  const std::vector<int> detectorList = getProperty("DetectorList");
  // only look at these other parameters if the file wasn't set
  if ( ! spectraList.empty() )
  {
    WorkspaceHelpers::getIndicesFromSpectra(
                                  workspace, spectraList, outputData[0]);
     g_log.debug() << "Converted " << spectraList.size() << " spectra numbers into spectra indexes to be combined" << std::endl;
  }
  else if ( ! detectorList.empty() )
  {// we are going to group on the basis of detector IDs, convert from detectors to spectra numbers
    std::vector<int> mySpectraList = workspace->spectraMap().getSpectra(detectorList);
    //then from spectra numbers to indices
    WorkspaceHelpers::getIndicesFromSpectra(
                                  workspace, mySpectraList, outputData[0]);
    g_log.debug() << "Found " << outputData[0].size() << " spectra indexes from the list of " << detectorList.size() << " detectors" << std::endl;
  }
  else if ( ! indexList.empty() )
  {
    outputData[0] = indexList;
    g_log.debug() << "Read in " << outputData[0].size() << " spectra indexs to be combined" << std::endl;
  }

  if ( outputData[0].empty() )
  {
    g_log.information() << name() << ": File, WorkspaceIndexList, SpectraList, and DetectorList properties are all empty" << std::endl;
    throw std::invalid_argument("All list properties are empty, nothing to group");
  }

  // up date unUsedSpec, this is used to find duplicates and when the user has set KeepUngroupedSpectra
  for ( int i = 0 ; i < outputData[0].size(); i++ )
  {
    if ( unUsedSpec[outputData[0][i]] != USED )
    {
      unUsedSpec[outputData[0][i]] = USED;
    }
    else g_log.warning() << "Duplicate index found" << std::endl;
  }
}
/** Read the spectra numbers in from the input file (the file format is in the
*  source file "GroupDetectors2.h" and make an array of spectra indexes to group
*  @param fname the full path name of the file to open
*  @param workspace a pointer to the input workspace, used to get spectra indexes from numbers
*  @param output the array that will contain the list of spectra indexes
*  @param unUsedSpec the list of spectra indexes that have been included in a group (so far)
*  @throws invalid_argument if there's any problem with the file or its format
*/
void GroupDetectors2::readTheFile( std::string fname,
                    Workspace2D_const_sptr workspace, storage_map &output,
                                            std::vector<int> &unUsedSpec )
{
  // tring to open the file the user told us exists, skip down 20 lines to find out what happens if we can read from it
  g_log.debug() << "Opening input file ... " << fname;
  std::ifstream File(fname.c_str(), std::ios::in);

  std::string firstLine;
  std::getline( File, firstLine );
  // for error reporting keep a count of where we are reading in the file
  int lineNum = 1;
  
  if ( File.rdstate() & std::ios::failbit ) 
  {
    g_log.debug() << " file state failbit set after reading attempt" << std::endl;
    throw std::invalid_argument(std::string("Couldn't read from file ") + fname);
  }
  g_log.debug() << " success opening input file " << fname << std::endl;
  progress(m_PercentDone += OPENINGFILE);
  // check for a (user) cancel message
  interruption_point();

  // allow spectra number to spectra index look ups
  std::map<int,int> specs2index;
  // if we could use a version of boost with bimap and converted to using it for index-spectra number lookups we wouldn't have to do this
  workspace->getAxis(1)->getSpectraIndexMap(specs2index);

  try
  {
    // we don't use the total number of groups report at the top of the file but we'll tell them later if there is a problem with it for their diagnostic purposes
    int totalNumberOfGroups = readInt(firstLine);
    
    // Reading file now ... 
    while ( totalNumberOfGroups == EMPTY_LINE )
    {
      if ( ! File ) throw std::invalid_argument("The input file doesn't appear to contain any data");
      std::getline( File, firstLine ), lineNum ++;
      totalNumberOfGroups = readInt(firstLine);
    }

    readGroups(specs2index, File, lineNum, output, unUsedSpec);

    if ( output.size() != totalNumberOfGroups )
    {
      g_log.warning() << "The input file header states there are " << totalNumberOfGroups << " but the file contains " << output.size() << " groups" << std::endl;
    }
  }
  // add some more info to the error messages, including the line number, to help users correct their files. These problems should cause the algorithm to stop
  catch (std::invalid_argument &e)
  {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(e.what() + std::string(" near line number ") + boost::lexical_cast<std::string>(lineNum));
    if ( File.rdstate() & std::ios::failbit )
      error = "Input output error while reading file ";
    throw std::invalid_argument(error);
  }
  catch (boost::bad_lexical_cast &e)
  {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(std::string("Problem reading numbers \"") + e.what() + std::string("\" near line number ") + boost::lexical_cast<std::string>(lineNum));
    if ( File.rdstate() & std::ios::failbit )
      error = "Input output error while reading file ";
    throw std::invalid_argument(error);
  }
  File.close();
  g_log.debug() << "Closed file " << fname << " after reading in " << output.size() << " groups" << std::endl;
  m_PercentDone += fileReadProg( output.size(), specs2index.size() );
  return;
}
/** The function expects that the string passed to it contains an integer number,
*  it reads the number and returns it
*  @param line a line read from the file, we'll interpret this
*  @throws invalid_argument when the line contains more just an integer
*  @throws boost::bad_lexical_cast when the string can't be interpreted as an integer
*/
int GroupDetectors2::readInt(std::string line)
{
  // remove comments and white space (TOK_TRIM)
  Poco::StringTokenizer dataComment(line, "#",Poco::StringTokenizer::TOK_TRIM);
  if ( dataComment.begin() != dataComment.end() )
  {
    Poco::StringTokenizer
      data(*(dataComment.begin()), " ", Poco::StringTokenizer::TOK_TRIM);
    if ( data.count() == 1 )
    {
      if ( ! data[0].empty() )
      {
        return boost::lexical_cast<int>(data[0]);
      }
    }
    else
    {
      // we expected an integer but there were more things on the line, before any #
      throw std::invalid_argument("Problem reading file, integer expected");
    }
  }
  // we haven't found any data, return the nodata condition
  return EMPTY_LINE;
}
/** The function expects that the string passed to it contains a serries of integers
*  it read them in and returns them
*  @param line a line read from the file, we'll interpret this
*  @param specs2index a map with spectra numbers as indexes and index numbers as values
*  @param output a list of spectra numbers in the group
*  @param unUsedSpec the list of spectra indexes that have been included in a group (so far)
*  @throws boost::bad_lexical_cast when the string contains a non-integer
*/
void GroupDetectors2::readSpectraIndexes(std::string line,
                std::map<int,int> &specs2index, std::vector<int> &output,
                std::vector<int> &unUsedSpec)
{
  // remove comments and white space (TOK_TRIM)
  Poco::StringTokenizer dataComment(line, "#",Poco::StringTokenizer::TOK_TRIM);
  if ( dataComment.begin() != dataComment.end() )
  {
//    std::string
//    replaceHyphens(dataComment, list);

    Poco::StringTokenizer
      data(*(dataComment.begin()), " ", Poco::StringTokenizer::TOK_TRIM);
    Poco::StringTokenizer::Iterator it = data.begin();
    for ( ; it != data.end(); ++it )
    {
      std::map<int, int>::const_iterator ind =
        specs2index.find(boost::lexical_cast<int>(*it));
      if ( ind == specs2index.end() )
      {
        g_log.debug() << name() << ": spectrum number " + *it + " refered to in the input file was not found in the input workspace" << std::endl;
        throw std::invalid_argument("Spectrum number " + *it + " not found");
      } 
      if ( unUsedSpec[ind->second] != USED )
      {// this array is used when the user sets KeepUngroupedSpectra, as well as to find duplicates
        unUsedSpec[ind->second] = USED;
        output.push_back( ind->second );
      }
      else
      {// the spectra was already included in a group
        g_log.warning() << "Duplicate spectra number " << ind->second << " ignored in input file" << std::endl;
      }
    }
  }
}
/** Reads from the file getting in order: an unused integer, on the next line the number of
*  spectra in the group and next one or more lines the spectra numbers, (format in GroupDetectors.h)
* @param specs2index a map that links spectra numbers to indexes
* @param File the input stream that is linked to the file
* @param lineNum the last line read in the file, is updated by this function
* @param output a map that will contain the list of spectra numbers, it is never index searched
* @param unUsedSpec list of spectra that haven't yet been included in a group
* @throws invalid_argument if there is any problem with the file
*/
void GroupDetectors2::readGroups(std::map<int,int> &specs2index,
             std::ifstream &File, int &lineNum, storage_map &output,
                                          std::vector<int> &unUsedSpec)
{
  // used in writing the spectra to the outData map
  int arbitaryMapKey = 0;
  // go through the rest of the file reading in lists of spectra number to group
  while ( File )
  {
    std::string thisLine;
    do
    {
      std::getline( File, thisLine ), lineNum ++;
      // we haven't started reading a new group and so if the file ends here it is OK
      if ( ! File ) return;
        // in some implementations this is the spectra number for the group but not here so we ignore the return value, uncomment out the stuff below to make it work
        // abitaryMapKey = readInt(thisLine);
    }//}
    while( readInt(thisLine) == EMPTY_LINE && File );  //while( abitaryMapKey == EMPTY_LINE && File );

    // the number of spectra that will be combined in the group
    int numberOfSpectra = EMPTY_LINE;
    do
    {
      if ( ! File ) throw std::invalid_argument("Premature end of file, expecting an integer with the number of spectra in the group");
      std::getline( File, thisLine ), lineNum ++;
      numberOfSpectra = readInt(thisLine);
    }
    while( numberOfSpectra == EMPTY_LINE );

    // the list of spectra numbers that will be combined into a group
    output[arbitaryMapKey].reserve(numberOfSpectra);
    do
    {
      if ( ! File ) throw std::invalid_argument("Premature end of file, found number of spectra specification but no spectra list");
      std::getline( File, thisLine ), lineNum ++;
      // the spectra numbers that will be included in the group
      readSpectraIndexes(
        thisLine, specs2index, output[arbitaryMapKey], unUsedSpec);
    }
    while (output[arbitaryMapKey].size() < numberOfSpectra);
    if ( output[arbitaryMapKey].size() != numberOfSpectra )
    {// it makes no sense to continue reading the file, we'll stop here
      throw std::invalid_argument(std::string("Bad number of spectra specification or spectra list near line number ") + boost::lexical_cast<std::string>(lineNum));
    }
    // make regular progress reports and check for a cancellastion notification
    if ( (output.size() % INTERVAL) == 1 )
    {
      fileReadProg( output.size(), specs2index.size() );
    }
    arbitaryMapKey ++;
  }
}

/** Called while reading input file to report progress (doesn't update m_percentdone ) and
*  check for algorithm cancel messages, doesn't look at file size to estimate progress
*  @param numGroupsRead number of groups read from the file so far (not the numer of spectra)
*  @param numInHists the total number of histograms in the input workspace
*  @return estimate of the amount of algorithm progress obtained by reading from the file
*/
double GroupDetectors2::fileReadProg(int numGroupsRead, int numInHists)
{
  // I'm going to guess that there are half as many groups as spectra
  double progEstim = numGroupsRead/numInHists;
  // but it might be more, in which case this complex function always increases but slower and slower
  progEstim = READFILE*progEstim*2/(1+progEstim);
  // now do the reporting
  progress(m_PercentDone + progEstim );
  // check for a (user) cancel message
  interruption_point();
  return progEstim;
}

int GroupDetectors2::makeGroups( const storage_map &inputList,
    Workspace2D_const_sptr inputWS, MatrixWorkspace_sptr outputWS,
                                   const double prog4Copy)
{
  // Get hold of the axis that holds the spectrum numbers
  Axis *inputSpecNums = inputWS->getAxis(1);
  // Get a reference to the spectra map on the output workspace
  SpectraDetectorMap &specDetecMap = outputWS->mutableSpectraMap();

  g_log.debug() << name() << ": Preparing to group spectra into " << inputList.size() << " groups" << std::endl;

  // where we are copying spectra to, we start copying to the start of the output workspace
  int outIndex = 0;
  storage_map::const_iterator it = inputList.begin();
  for ( ; it != inputList.end() ; ++it )
  {
    // get the spectra number for the first spectrum in the list to be grouped
    const int firstSpecNum =
      inputSpecNums->spectraNo(it->second.front());
    // the spectrum number of new group will be the number of the spectrum number of first spectrum that was grouped
    outputWS->getAxis(1)->spectraNo(outIndex) = firstSpecNum;

    // Copy over X data from first spectrum, the bin boundaries for all spectra are assumed to be the same here
    outputWS->dataX(outIndex) = inputWS->readX(0);
    // the Y values and errors from spectra being grouped are combined in the output spectrum
    std::vector<int>::const_iterator specIt = it->second.begin();
    for( ; specIt != it->second.end(); ++specIt)
    {
      const int copyFrom = *specIt;
      // Move the current detector to belong to the first spectrum
      specDetecMap.remap(inputSpecNums->spectraNo(copyFrom), firstSpecNum);
      // Add up all the Y spectra and store the result in the first one
      // Need to keep the next 3 lines inside loop for now until ManagedWorkspace mru-list works properly
      MantidVec &firstY = outputWS->dataY(outIndex);
      MantidVec::iterator fYit;
      MantidVec::iterator fEit = outputWS->dataE(outIndex).begin();
      MantidVec::const_iterator Yit = inputWS->readY(copyFrom).begin();
      MantidVec::const_iterator Eit = inputWS->readE(copyFrom).begin();
      for (fYit = firstY.begin(); fYit != firstY.end(); ++fYit, ++fEit, ++Yit, ++Eit)
      {
        *fYit += *Yit;
        // Assume 'normal' (i.e. Gaussian) combination of errors
        *fEit = std::sqrt( (*fEit)*(*fEit) + (*Eit)*(*Eit) );
      }
      // make regular progress reports and check for cancelling the algorithm
      if ( outIndex % INTERVAL == 0 )
      {
        m_PercentDone += INTERVAL*prog4Copy;
        if ( m_PercentDone > 100 )
        {
          m_PercentDone = 100;
        }
        progress(m_PercentDone);
        interruption_point();
      }
    }
    outIndex ++;
  }
  g_log.debug() << name() << " created " << outIndex << " new grouped spectra" << std::endl;
  return outIndex;
}


void GroupDetectors2::moveOthers(const std::set<int> &unGroupedSet,
       Workspace2D_const_sptr inputWS, MatrixWorkspace_sptr outputWS,
                                                   int outIndex)
{
  g_log.debug() << "Starting to copy the ungrouped spectra" << std::endl;
  double prog4Copy =static_cast<double>(100-m_PercentDone)/unGroupedSet.size();

  std::set<int>::const_iterator copyFrIt = unGroupedSet.begin();
  // move passed the one GroupDetectors2::USED value at the start of the set
  copyFrIt ++;
  // go thorugh all the spectra in the input workspace
  for ( ; copyFrIt != unGroupedSet.end(); ++copyFrIt )
  {
    if ( outIndex == outputWS->getNumberHistograms() )
/*STEVE get rid of this*/throw std::logic_error("Couldn't copy all of the spectra into the output workspace");

    outputWS->dataX(outIndex) = inputWS->readX(*copyFrIt);
    outputWS->dataY(outIndex) = inputWS->readY(*copyFrIt);
    outputWS->dataE(outIndex) = inputWS->readE(*copyFrIt);
    outputWS->getAxis(1)->spectraNo(outIndex) =
      inputWS->getAxis(1)->spectraNo(*copyFrIt);
    // go to the next free index in the output workspace
    outIndex ++;
    // make regular progress reports and check for cancelling the algorithm
    if ( outIndex % INTERVAL == 0 )
    {
      m_PercentDone += INTERVAL*prog4Copy;
      if ( m_PercentDone > 100 )
      {
        m_PercentDone = 100;
      }
      progress(m_PercentDone);
      interruption_point();
    }
  }
  g_log.debug() << name() << " copied " << unGroupedSet.size() << " ungrouped spectra" << std::endl;
}

} // namespace DataHandling
} // namespace Mantid
