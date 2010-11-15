//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
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

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors2)

using namespace Kernel;
using namespace API;

/// (Empty) Constructor
GroupDetectors2::GroupDetectors2() : m_FracCompl(0.0) {}

/// Destructor
GroupDetectors2::~GroupDetectors2() {}

// progress estimates
const double GroupDetectors2::CHECKBINS = 0.10;  
const double GroupDetectors2::OPENINGFILE = 0.03;
// if CHECKBINS+OPENINGFILE+2*READFILE > 1 then the algorithm might report progress > 100%
const double GroupDetectors2::READFILE = 0.15;

void GroupDetectors2::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
    new CommonBinsValidator<>),"The name of the input 2D workspace");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the output workspace");
  std::vector<std::string> fileExts(2);
  fileExts[0] = ".map";
  fileExts[1] = ".xml";
  declareProperty(new FileProperty("MapFile", "", FileProperty::OptionalLoad, fileExts),
    "A file that consists of lists of spectra numbers to group. See the help\n"
    "for the file format");
  declareProperty(new ArrayProperty<int>("SpectraList"),
    "An array containing a list of the spectrum numbers to combine\n"
    "(DetectorList and WorkspaceIndexList are ignored if this is set)" );
  declareProperty(new ArrayProperty<int>("DetectorList"), 
    "An array of detector IDs to combine (WorkspaceIndexList is ignored if this is\n"
    "set)" );
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "An array of workspace indices to combine" );
  declareProperty("KeepUngroupedSpectra",false,
    "If true ungrouped spectra will be copied to the output workspace\n"
    "and placed after the groups");
}

void GroupDetectors2::exec()
{
  // Get the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int numInHists = inputWS->getNumberHistograms();
  // Bin boundaries need to be the same, so do the full check on whether they actually are
  if (!API::WorkspaceHelpers::commonBoundaries(inputWS))
  {
    g_log.error() << "Can only group if the histograms have common bin boundaries\n";
    throw std::invalid_argument("Can only group if the histograms have common bin boundaries");
  }
  progress( m_FracCompl = CHECKBINS );
  interruption_point();
  
  // There may be alot of spectra so listing the the ones that aren't grouped could be a big deal
  std::vector<int> unGroupedInds;
  unGroupedInds.reserve(numInHists);
  for( int i = 0; i < numInHists ; i++ )
  {
    unGroupedInds.push_back(i);
  }

  // read in the input parameters to make that map, if KeepUngroupedSpectra was set we'll need a list of the ungrouped spectrra too
  getGroups(inputWS, unGroupedInds);

  // converting the list into a set gets rid of repeated values, here the multiple GroupDetectors2::USED become one USED at the start
  const std::set<int> unGroupedSet(unGroupedInds.begin(), unGroupedInds.end());

  // Check what the user asked to be done with ungrouped spectra
  const bool keepAll = getProperty("KeepUngroupedSpectra");
  // ignore the one USED value in set or ignore all the ungrouped if the user doesn't want them
  const int numUnGrouped = keepAll ? unGroupedSet.size()-1 : 0;

  MatrixWorkspace_sptr outputWS =
    WorkspaceFactory::Instance().create(inputWS,
    m_GroupSpecInds.size()+ numUnGrouped, inputWS->readX(0).size(),
    inputWS->blocksize());

  // prepare to move the requested histograms into groups, first estimate how long for progress reporting. +1 in the demonator gets rid of any divide by zero risk
  double prog4Copy=( (1.0 - m_FracCompl)/(numInHists-unGroupedSet.size()+1) )*
    (keepAll ? static_cast<float>(numInHists-unGroupedSet.size())/numInHists: 1);
  // now do all the moving
  int outIndex = formGroups(inputWS, outputWS, prog4Copy);
    
  // If we're keeping ungrouped spectra
  if (keepAll)
  {
    // copy them into the output workspace
    moveOthers(unGroupedSet, inputWS, outputWS, outIndex);
  }

  g_log.information() << name() << " algorithm has finished\n";

  setProperty("OutputWorkspace",outputWS);
}

/** Make a map containing spectra indexes to group, the indexes could have come from
*  file, or an array, spectra numbers ...
*  @param workspace the user selected input workspace
*  @param unUsedSpec spectra indexes that are members of any group
*/
void GroupDetectors2::getGroups(API::MatrixWorkspace_const_sptr workspace,
                       std::vector<int> &unUsedSpec)
{
  // this is the map that we are going to fill
  m_GroupSpecInds.clear();

  // There are several properties that may contain the user data go through them in order of precedence
  const std::string filename = getProperty("MapFile");
  if ( ! filename.empty() )
  {// The file property has been set so try to load the file
    try
    {
      // check if XML file and if yes assume it is a XML grouping file
      std::string filenameCopy(filename);
      std::transform(filenameCopy.begin(), filenameCopy.end(), filenameCopy.begin(), tolower);
      if ( (filenameCopy.find(".xml")) != std::string::npos )
      {
        processXMLFile(filename, workspace, unUsedSpec);
      }
      else
      {
        // the format of this input file format is described in "GroupDetectors2.h"
        processFile(filename, workspace, unUsedSpec);
      }
    }
    catch ( std::exception & )
    {
      g_log.error() << name() << ": Error reading input file " << filename << std::endl;
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
    workspace->getIndicesFromSpectra( spectraList, m_GroupSpecInds[0]);
    g_log.debug() << "Converted " << spectraList.size() << " spectra numbers into spectra indices to be combined\n";
  }
  else
  {// go thorugh the rest of the properties in order of decreasing presidence, abort when we get the data we need ignore the rest
    if ( ! detectorList.empty() )
    {// we are going to group on the basis of detector IDs, convert from detectors to spectra numbers
      std::vector<int> mySpectraList = workspace->spectraMap().getSpectra(detectorList);
      //then from spectra numbers to indices
      workspace->getIndicesFromSpectra( mySpectraList, m_GroupSpecInds[0]);
      g_log.debug() << "Found " << m_GroupSpecInds[0].size() << " spectra indices from the list of " << detectorList.size() << " detectors\n";
    }
    else if ( ! indexList.empty() )
    {
      m_GroupSpecInds[0] = indexList;
      g_log.debug() << "Read in " << m_GroupSpecInds[0].size() << " spectra indices to be combined\n";
    }
    //check we don't have an index that is too high for the workspace
    int maxIn = workspace->getNumberHistograms() - 1;
    std::vector<int>::const_iterator it = m_GroupSpecInds[0].begin();
    for( ; it != m_GroupSpecInds[0].end() ; ++it )
    {
      if ( *it > maxIn )
      {
        g_log.error() << "Spectra index " << *it << " doesn't exist in the input workspace, the highest possible index is " << maxIn << std::endl;
        throw std::out_of_range("One of the spectra requested to group does not exist in the input workspace");
      }
    }
  }

  if ( m_GroupSpecInds[0].empty() )
  {
    g_log.information() << name() << ": File, WorkspaceIndexList, SpectraList, and DetectorList properties are all empty\n";
    throw std::invalid_argument("All list properties are empty, nothing to group");
  }

  // up date unUsedSpec, this is used to find duplicates and when the user has set KeepUngroupedSpectra
  std::vector<int>::const_iterator index = m_GroupSpecInds[0].begin(); 
  for (  ; index != m_GroupSpecInds[0].end(); ++index )
  {// the vector<int> m_GroupSpecInds[0] must not index contain numbers that don't exist in the workspaace
    if ( unUsedSpec[*index] != USED )
    {
      unUsedSpec[*index] = USED;
    }
    else g_log.warning() << "Duplicate index, " << *index << ", found\n";
  }
}
/** Read the spectra numbers in from the input file (the file format is in the
*  source file "GroupDetectors2.h" and make an array of spectra indexes to group
*  @param fname the full path name of the file to open
*  @param workspace a pointer to the input workspace, used to get spectra indexes from numbers
*  @param unUsedSpec the list of spectra indexes that have been included in a group (so far)
*  @throws FileError if there's any problem with the file or its format
*/
void GroupDetectors2::processFile(std::string fname,
  API::MatrixWorkspace_const_sptr workspace, std::vector<int> &unUsedSpec)
{
  // tring to open the file the user told us exists, skip down 20 lines to find out what happens if we can read from it
  g_log.debug() << "Opening input file ... " << fname;
  std::ifstream File(fname.c_str(), std::ios::in);

  std::string firstLine;
  std::getline( File, firstLine );
  // for error reporting keep a count of where we are reading in the file
  int lineNum = 1;
  
  if (File.fail()) 
  {
    g_log.debug() << " file state failbit set after read attempt\n";
    throw Exception::FileError("Couldn't read file", fname);
  }
  g_log.debug() << " success opening input file " << fname << std::endl;
  progress(m_FracCompl += OPENINGFILE);
  // check for a (user) cancel message
  interruption_point();

  // allow spectra number to spectra index look ups
  std::map<int,int> specs2index;
  const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(workspace->getAxis(1));
  if (axis)
  {
    axis->getSpectraIndexMap(specs2index);
  }

  try
  {
    // we don't use the total number of groups report at the top of the file but we'll tell them later if there is a problem with it for their diagnostic purposes
    int totalNumberOfGroups = readInt(firstLine);
    
    // Reading file now ... 
    while ( totalNumberOfGroups == EMPTY_LINE )
    {
      if ( ! File ) throw Exception::FileError("The input file doesn't appear to contain any data", fname);
      std::getline( File, firstLine ), lineNum ++;
      totalNumberOfGroups = readInt(firstLine);
    }

    readFile(specs2index, File, lineNum, unUsedSpec);

    if ( m_GroupSpecInds.size() != static_cast<size_t>(totalNumberOfGroups) )
    {
      g_log.warning() << "The input file header states there are " << totalNumberOfGroups << " but the file contains " << m_GroupSpecInds.size() << " groups\n";
    }
  }
  // add some more info to the error messages, including the line number, to help users correct their files. These problems should cause the algorithm to stop
  catch (std::invalid_argument &e)
  {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(e.what() + std::string(" near line number ") + boost::lexical_cast<std::string>(lineNum));
    if (File.fail())
    {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  }
  catch (boost::bad_lexical_cast &e)
  {
    g_log.debug() << "Exception thrown: " << e.what() << std::endl;
    File.close();
    std::string error(std::string("Problem reading integer value \"") + e.what() + std::string("\" near line number ") + boost::lexical_cast<std::string>(lineNum));
    if (File.fail())
    {
      error = "Input output error while reading file ";
    }
    throw Exception::FileError(error, fname);
  }
  File.close();
  g_log.debug() << "Closed file " << fname << " after reading in " << m_GroupSpecInds.size() << " groups\n";
  m_FracCompl += fileReadProg( m_GroupSpecInds.size(), specs2index.size() );
  return;
}

/** Get groupings from XML file
*
*  @param fname the full path name of the file to open
*  @param workspace a pointer to the input workspace, used to get spectra indexes from numbers
*  @param unUsedSpec the list of spectra indexes that have been included in a group (so far)
*  @throws FileError if there's any problem with the file or its format
*/
void GroupDetectors2::processXMLFile(std::string fname,
  API::MatrixWorkspace_const_sptr workspace, std::vector<int> &unUsedSpec)
{
  // tring to open the file the user told us exists, skip down 20 lines to find out what happens if we can read from it
  g_log.debug() << "Opening input file ... " << fname;

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(fname);
  }
  catch(...)
  {
    g_log.error("Unable to parse file " + fname);
    throw Kernel::Exception::FileError("Unable to parse File:" , fname);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if ( !pRootElem->hasChildNodes() )
  {
    g_log.error("XML file: " + fname + "contains no root element.");
    throw Kernel::Exception::FileError("No root element in XML grouping file:" , fname);
  }  

  NodeList* pNL_group = pRootElem->getElementsByTagName("group");
  if ( pNL_group->length() == 0 )
  {
    g_log.error("XML group file: " + fname + "contains no group elements.");
    throw Kernel::Exception::FileError("XML group file contains no group elements:" , fname);
  }

  // allow spectra number to spectra index look ups
  std::map<int,int> specs2index;
  const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(workspace->getAxis(1));
  if (axis)
  {
    axis->getSpectraIndexMap(specs2index);
  }

  unsigned int numberGroups = pNL_group->length();
  for (unsigned int iGroup = 0; iGroup < numberGroups; iGroup++)
  {
    Element* pGroupElem = static_cast<Element*>(pNL_group->item(iGroup));
    
    Element* idlistElement = pGroupElem->getChildElement("ids");
    if (idlistElement)
    {
      std::string ids = idlistElement->getAttribute("val");

      // the spectra numbers that will be included in the group
      readSpectraIndexes(ids, specs2index, m_GroupSpecInds[iGroup], unUsedSpec, ",");
    }
    else
    {
      g_log.error("XML group file: " + fname + "contains no <ids> elements.");
      throw Kernel::Exception::FileError("XML group file contains no <ids> elements:" , fname);
    }
   
  }
  pNL_group->release();
}

/** The function expects that the string passed to it contains an integer number,
*  it reads the number and returns it
*  @param line a line read from the file, we'll interpret this
*  @return the integer read from the line, error code if not readable
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
        try
        {
          return boost::lexical_cast<int>(data[0]);
        }
        catch (boost::bad_lexical_cast &e)
        {
          g_log.debug() << "Exception thrown: " << e.what() << std::endl;
          throw std::invalid_argument("Error reading file, integer expected");
        }
      }
    }
    else
    {
      if ( data.count() == 0 )
      {
        return EMPTY_LINE;
      }
      // we expected an integer but there were more things on the line, before any #
      g_log.debug() << "Error: found " << data.count() << " strings the first string is " << data[0] << std::endl;
      throw std::invalid_argument("Problem reading file, a singe integer expected");
    }
  }
  // we haven't found any data, return the nodata condition
  return EMPTY_LINE;
}
/** Reads from the file getting in order: an unused integer, on the next line the number of
*  spectra in the group and next one or more lines the spectra numbers, (format in GroupDetectors.h)
* @param specs2index a map that links spectra numbers to indexes
* @param File the input stream that is linked to the file
* @param lineNum the last line read in the file, is updated by this function
* @param unUsedSpec list of spectra that haven't yet been included in a group
* @throws invalid_argument if there is any problem with the file
*/
void GroupDetectors2::readFile(std::map<int,int> &specs2index, std::ifstream &File, int &lineNum, std::vector<int> &unUsedSpec)
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

    // the value of this map is the list of spectra numbers that will be combined into a group
    m_GroupSpecInds[arbitaryMapKey].reserve(numberOfSpectra);
    do
    {
      if ( ! File ) throw std::invalid_argument("Premature end of file, found number of spectra specification but no spectra list");
      std::getline( File, thisLine ), lineNum ++;
      // the spectra numbers that will be included in the group
      readSpectraIndexes(
        thisLine, specs2index, m_GroupSpecInds[arbitaryMapKey], unUsedSpec);
    }
    while (static_cast<int>(m_GroupSpecInds[arbitaryMapKey].size()) < numberOfSpectra);
    if ( static_cast<int>(m_GroupSpecInds[arbitaryMapKey].size()) != numberOfSpectra )
    {// it makes no sense to continue reading the file, we'll stop here
      throw std::invalid_argument(std::string("Bad number of spectra specification or spectra list near line number ") + boost::lexical_cast<std::string>(lineNum));
    }
    // make regular progress reports and check for a cancellastion notification
    if ( (m_GroupSpecInds.size() % INTERVAL) == 1 )
    {
      fileReadProg( m_GroupSpecInds.size(), specs2index.size() );
    }
    arbitaryMapKey ++;
  }
}
/** The function expects that the string passed to it contains a series of integers,
*  ranges specified with a '-' are possible
*  @param line a line read from the file, we'll interpret this
*  @param specs2index a map with spectra numbers as indexes and index numbers as values
*  @param output the list of integers, with any ranges expanded
*  @param unUsedSpec the list of spectra indexes that have been included in a group (so far)
*  @param seperator the symbol for the index range separator
*  @throws invalid_argument when a number couldn't be found or the number is not in the spectra map
*/
void GroupDetectors2::readSpectraIndexes(std::string line, std::map<int,int> &specs2index, std::vector<int> &output, std::vector<int> &unUsedSpec, std::string seperator)
{
  // remove comments and white space
  Poco::StringTokenizer dataComment(line, seperator, IGNORE_SPACES);
  Poco::StringTokenizer::Iterator iend = dataComment.end();
  for( Poco::StringTokenizer::Iterator itr = dataComment.begin(); itr != iend; ++itr )
  {
    std::vector<int> specNums;
    specNums.reserve(output.capacity());

    RangeHelper::getList(*itr, specNums);
    
    std::vector<int>::const_iterator specN = specNums.begin();
    for( ;specN!=specNums.end(); ++specN)
    {
      std::map<int, int>::const_iterator ind = specs2index.find(*specN);
      if ( ind == specs2index.end() )
      {
        g_log.debug() << name() << ": spectrum number " << *specN << " refered to in the input file was not found in the input workspace\n";
        throw std::invalid_argument("Spectrum number " + boost::lexical_cast<std::string>(*specN) + " not found");
      } 
      if ( unUsedSpec[ind->second] != USED )
      {// this array is used when the user sets KeepUngroupedSpectra, as well as to find duplicates
        unUsedSpec[ind->second] = USED;
        output.push_back( ind->second );
      }
      else
      {// the spectra was already included in a group
        g_log.warning() << "Duplicate spectra number " << *specN << " ignored in input file\n";
      }
    }
  }
}
/** Called while reading input file to report progress (doesn't update m_FracCompl ) and
*  check for algorithm cancel messages, doesn't look at file size to estimate progress
*  @param numGroupsRead number of groups read from the file so far (not the number of spectra)
*  @param numInHists the total number of histograms in the input workspace
*  @return estimate of the amount of algorithm progress obtained by reading from the file
*/
double GroupDetectors2::fileReadProg(int numGroupsRead, int numInHists)
{
  // I'm going to guess that there are half as many groups as spectra
  double progEstim = 2*numGroupsRead/static_cast<float>(numInHists);
  // but it might be more, in which case this complex function always increases but slower and slower
  progEstim = READFILE*progEstim/(1+progEstim);
  // now do the reporting
  progress(m_FracCompl + progEstim );
  // check for a (user) cancel message
  interruption_point();
  return progEstim;
}
/** Move the user selected spectra in the input workspace into groups in the output workspace
*  @param inputWS user selected input workspace for the algorithm
*  @param outputWS user selected output workspace for the algorithm
*  @param prog4Copy the amount of algorithm progress to attribute to moving a single spectra
*  @return number of new grouped spectra
*/
int GroupDetectors2::formGroups( API::MatrixWorkspace_const_sptr inputWS, API::MatrixWorkspace_sptr outputWS, const double prog4Copy)
{
  // Get hold of the axis that holds the spectrum numbers
  Axis *inputSpecNums = inputWS->getAxis(1);
  // Get a reference to the spectra map on the output workspace
  API::SpectraDetectorMap &specDetecMap = outputWS->mutableSpectraMap();

  g_log.debug() << name() << ": Preparing to group spectra into " << m_GroupSpecInds.size() << " groups\n";

  // where we are copying spectra to, we start copying to the start of the output workspace
  int outIndex = 0;
  storage_map::const_iterator it = m_GroupSpecInds.begin();
  for ( ; it != m_GroupSpecInds.end() ; ++it )
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
    }
    // make regular progress reports and check for cancelling the algorithm
    if ( outIndex % INTERVAL == 0 )
    {
      m_FracCompl += INTERVAL*prog4Copy;
      if ( m_FracCompl > 1.0 )
      {
        m_FracCompl = 1.0;
      }
      progress(m_FracCompl);
      interruption_point();
    }
    outIndex ++;
  }
  g_log.debug() << name() << " created " << outIndex << " new grouped spectra\n";
  return outIndex;
}
/** Only to be used if the KeepUnGrouped property is true, moves the spectra that were not selected
* to be in a group to the end of the output spectrum
*  @param unGroupedSet list of spectra indexes that were included in a group
*  @param inputWS user selected input workspace for the algorithm
*  @param outputWS user selected output workspace for the algorithm
*  @param outIndex the next spectra index available after the grouped spectra
*/
void GroupDetectors2::moveOthers(const std::set<int> &unGroupedSet, API::MatrixWorkspace_const_sptr inputWS, API::MatrixWorkspace_sptr outputWS, int outIndex)
{
  g_log.debug() << "Starting to copy the ungrouped spectra" << std::endl;
  double prog4Copy = static_cast<float>(1.0-m_FracCompl)/unGroupedSet.size();

  std::set<int>::const_iterator copyFrIt = unGroupedSet.begin();
  // move passed the one GroupDetectors2::USED value at the start of the set
  copyFrIt ++;
  // go thorugh all the spectra in the input workspace
  for ( ; copyFrIt != unGroupedSet.end(); ++copyFrIt )
  {// error checking code that code be added if ( outIndex == outputWS->getNumberHistograms() ) throw std::logic_error("Couldn't copy all of the spectra into the output workspace");
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
      m_FracCompl += INTERVAL*prog4Copy;
      if ( m_FracCompl > 1.0 )
      {
        m_FracCompl = 1.0;
      }
      progress(m_FracCompl);
      interruption_point();
    }
  }
  g_log.debug() << name() << " copied " << unGroupedSet.size()-1 << " ungrouped spectra\n";
}
//RangeHelper
/** Expands any ranges in the input string, eg. "1 3-5 4" -> "1 3 4 5 4"
*  @param line a line of input that is interpreted and expanded
*  @param outList all integers specified both as ranges and individually in order
*  @throws invalid_argument if a character is found that is not an integer or hypehn and when a hyphen occurs at the start or the end of the line
*/
void GroupDetectors2::RangeHelper::getList(const std::string &line, std::vector<int> &outList)
{
  if ( line.empty() )
  {// it is not an error to have an empty line but it would cause problems with an error check a the end of this function
    return;
  }
  Poco::StringTokenizer ranges(line, "-");

  unsigned int loop = 0;
  try
  {
    do
    {
      Poco::StringTokenizer beforeHyphen(ranges[loop], " ", IGNORE_SPACES);
      Poco::StringTokenizer::Iterator readPostion = beforeHyphen.begin();
      if ( readPostion == beforeHyphen.end() )
      {
        throw std::invalid_argument("'-' found at the start of a list, can't interpret range specification");
      }
      for ( ; readPostion != beforeHyphen.end(); ++readPostion )
      {
        outList.push_back(boost::lexical_cast<int>(*readPostion));
      }
      // this will be the start of a range if it was followed by a - i.e. another token was captured
      const int rangeStart = outList.back();
      if (loop+1 == ranges.count())
      {// there is no more input
        break;
      }

      Poco::StringTokenizer afterHyphen(ranges[loop+1], " ", IGNORE_SPACES);
      readPostion = afterHyphen.begin();
      if ( readPostion == afterHyphen.end() )
      {
        throw std::invalid_argument("A '-' follows straight after another '-', can't interpret range specification");
      }

      // the tokenizer will always return at least on string
      const int rangeEnd = boost::lexical_cast<int>(*readPostion);
    
      // this is unanticipated and marked as an error, it would be easy to change this to count down however
      if ( rangeStart > rangeEnd )
      {
        throw std::invalid_argument("A range where the first integer is larger than the second is not allowed");
      }
      
      // expand the range
      for ( int j = rangeStart+1; j < rangeEnd; j++ )
      {
        outList.push_back(j);
      }
      
      loop ++;
    }
    while( loop < ranges.count() );
  }
  catch (boost::bad_lexical_cast &e)
  {
    throw std::invalid_argument(std::string("Expected list of integers, exception thrown: ") + e.what() );
  }
  if ( *(line.end()-1) == '-' )
  {
    throw std::invalid_argument("'-' found at the end of a list, can't interpret range specification");
  }
}

} // namespace DataHandling
} // namespace Mantid
