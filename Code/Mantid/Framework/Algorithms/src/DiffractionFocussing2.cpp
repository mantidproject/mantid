//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Axis.h"
#include <cfloat>
#include <fstream>
#include <iterator>
#include <numeric>
#include "MantidAPI/ISpectrum.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::vector;

namespace Mantid
{

namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)



/// Constructor
DiffractionFocussing2::DiffractionFocussing2() : 
  API::Algorithm(), udet2group(), groupAtWorkspaceIndex(), group2xvector(),
  group2wgtvector(), nGroups(0), nHist(0), nPoints(0)
{}

/// Destructor
DiffractionFocussing2::~DiffractionFocussing2()
{}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void DiffractionFocussing2::init()
{

  auto wsValidator = boost::make_shared<CompositeValidator>();
  // wsValidator->add<wsValidator->add>("dSpacing");
  wsValidator->add<API::RawCountValidator>();
  declareProperty(
    new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A 2D workspace with X values of d-spacing/Q-spacing" );
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );

  declareProperty(new FileProperty("GroupingFileName", "", FileProperty::OptionalLoad, ".cal"),
      "Optional: The name of the CalFile with grouping data." );

  declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: GroupingWorkspace to use instead of a grouping file." );

  declareProperty("PreserveEvents", true, "Keep the output workspace as an EventWorkspace, if the input has events (default).\n"
      "If false, then the workspace gets converted to a Workspace2D histogram.");
}


//=============================================================================
/** Perform clean-up of memory after execution but before destructor.
 * Private method
 */
void DiffractionFocussing2::cleanup()
{
  //Clear maps and vectors to free up memory.
  udet2group.clear();
  groupAtWorkspaceIndex.clear();
  std::vector<int>().swap(groupAtWorkspaceIndex);
  group2xvector.clear();
  group2wgtvector.clear();
  this->m_validGroups.clear();
  this->m_wsIndices.clear();
}


//=============================================================================
/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void DiffractionFocussing2::exec()
{
  // retrieve the properties
  std::string groupingFileName=getProperty("GroupingFileName");
  groupWS = getProperty("GroupingWorkspace");

  if (!groupingFileName.empty() && groupWS)
    throw std::invalid_argument("You must enter a GroupingFileName or a GroupingWorkspace, not both!");

  if (groupingFileName.empty() && !groupWS)
    throw std::invalid_argument("You must enter a GroupingFileName or a GroupingWorkspace!");

  // Get the input workspace
  m_matrixInputW = getProperty("InputWorkspace");
  nPoints = static_cast<int>(m_matrixInputW->blocksize());
  nHist = static_cast<int>(m_matrixInputW->getNumberHistograms());

  // Validate UnitID (spacing)
  Axis* axis = m_matrixInputW->getAxis(0);
  std::string unitid = axis->unit()->unitID();
  if (unitid != "dSpacing" && unitid != "MomentumTransfer" && unitid != "TOF"){
    g_log.error() << "UnitID " << unitid << " is not a supported spacing" << std::endl;
    throw new std::invalid_argument("Workspace Invalid Spacing/UnitID");
  }
  // --- Do we need to read the grouping workspace? ----
  if (groupingFileName != "")
  {
    progress(0.01, "Reading grouping file");
    IAlgorithm_sptr childAlg = createChildAlgorithm("CreateGroupingWorkspace");
    childAlg->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_matrixInputW));
    childAlg->setProperty("OldCalFilename", groupingFileName);
    childAlg->executeAsChildAlg();
    groupWS = childAlg->getProperty("OutputWorkspace");
  }

  // Fill the map
  progress(0.2, "Determine Rebin Params");
  udet2group.clear();
  // std::cout << "(1) nGroups " << nGroups << "\n";
  groupWS->makeDetectorIDToGroupVector(udet2group, nGroups);
  if (nGroups <= 0)
    throw std::runtime_error("No groups were specified.");
  // std::cout << "(2) nGroups " << nGroups << "\n";

  //This finds the rebin parameters (used in both versions)
  // It also initializes the groupAtWorkspaceIndex[] array.
  determineRebinParameters();

  size_t totalHistProcess = this->setupGroupToWSIndices();

  // determine event workspace min/max tof
  double eventXMin = 0.;
  double eventXMax = 0.;

  m_eventW = boost::dynamic_pointer_cast<const EventWorkspace>( m_matrixInputW );
  if (m_eventW != NULL)
  {
    if (getProperty("PreserveEvents"))
    {
      //Input workspace is an event workspace. Use the other exec method
      this->execEvent();
      this->cleanup();
      return;
    }
    else
    {
      // get the full d-spacing range
      m_eventW->sortAll(DataObjects::TOF_SORT, NULL);
      m_matrixInputW->getXMinMax(eventXMin, eventXMax);
    }
  }

  //No problem! It is a normal Workspace2D
  API::MatrixWorkspace_sptr out=API::WorkspaceFactory::Instance().create(m_matrixInputW,nGroups,nPoints+1,nPoints);
  // Caching containers that are either only read from or unused. Initialize them once.
  // Helgrind will show a race-condition but the data is completely unused so it is irrelevant
  MantidVec weights_default(1,1.0), emptyVec(1,0.0), EOutDummy(nPoints);

  Progress * prog;
  prog = new API::Progress(this, 0.2, 1.00, static_cast<int>(totalHistProcess) + nGroups);
#ifndef __APPLE__
  PARALLEL_FOR2(m_matrixInputW, out)
#endif
  for (int outWorkspaceIndex = 0; outWorkspaceIndex < static_cast<int>(m_validGroups.size()); outWorkspaceIndex++)
  {
    PARALLEL_START_INTERUPT_REGION
    int group = m_validGroups[outWorkspaceIndex];

    // Get the group
    group2vectormap::iterator it=group2xvector.find(group);
    group2vectormap::difference_type dif=std::distance(group2xvector.begin(),it);
    const MantidVec& Xout = *((*it).second);

    // Assign the new X axis only once (i.e when this group is encountered the first time)
    out->dataX(static_cast<int64_t>(dif))=Xout;

    // This is the output spectrum
    ISpectrum * outSpec = out->getSpectrum(outWorkspaceIndex);

    // Also set the spectrum number to the group number
    outSpec->setSpectrumNo(group);
    outSpec->clearDetectorIDs();

    // Get the references to Y and E output and rebin
    MantidVec& Yout=outSpec->dataY();
    MantidVec& Eout=outSpec->dataE();

    // Initialize the group's weight vector here and the dummy vector used for accumulating errors.
    MantidVec groupWgt(nPoints,0.0);

    // loop through the contributing histograms
    std::vector<size_t> indices = m_wsIndices[group];
    const size_t groupSize = indices.size();
    for (size_t i=0; i<groupSize; i++)
    {
      size_t inWorkspaceIndex = indices[i];
      // This is the input spectrum
      const ISpectrum * inSpec = m_matrixInputW->getSpectrum(inWorkspaceIndex);
      //Get reference to its old X,Y,and E.
      const MantidVec& Xin=inSpec->readX();
      const MantidVec& Yin=inSpec->readY();
      const MantidVec& Ein=inSpec->readE();

      outSpec->addDetectorIDs( inSpec->getDetectorIDs() );
      try
      {
        VectorHelper::rebinHistogram(Xin,Yin,Ein,Xout,Yout,Eout,true);
      }catch(...)
      {
        // Should never happen because Xout is constructed to envelop all of the Xin vectors
        std::ostringstream mess;
        mess << "Error in rebinning process for spectrum:" << inWorkspaceIndex;
        throw std::runtime_error(mess.str());
      }

      // Check for masked bins in this spectrum
      if ( m_matrixInputW->hasMaskedBins(i) )
      {
        MantidVec weight_bins,weights;
        weight_bins.push_back(Xin.front());
        // If there are masked bins, get a reference to the list of them
        const API::MatrixWorkspace::MaskList& mask = m_matrixInputW->maskedBins(i);
        // Now iterate over the list, adjusting the weights for the affected bins
        for (API::MatrixWorkspace::MaskList::const_iterator it = mask.begin(); it!= mask.end(); ++it)
        {
          const double currentX = Xin[(*it).first];
          // Add an intermediate bin with full weight if masked bins aren't consecutive
          if (weight_bins.back() != currentX)
          {
            weights.push_back(1.0);
            weight_bins.push_back(currentX);
          }
          // The weight for this masked bin is 1 - the degree to which this bin is masked
          weights.push_back(1.0-(*it).second);
          weight_bins.push_back(Xin[(*it).first + 1]);
        }
        // Add on a final bin with full weight if masking doesn't go up to the end
        if (weight_bins.back() != Xin.back())
        {
          weights.push_back(1.0);
          weight_bins.push_back(Xin.back());
        }

        // Create a zero vector for the errors because we don't care about them here
        const MantidVec zeroes(weights.size(),0.0);
        // Rebin the weights - note that this is a distribution
        VectorHelper::rebin(weight_bins,weights,zeroes,Xout,groupWgt,EOutDummy,true,true);
      }
      else // If no masked bins we want to add 1 to the weight of the output bins that this input covers
      {
        // Initialized within the loop to avoid having to wrap writing to it with a PARALLEL_CRITICAL sections
        MantidVec limits(2);

        if (eventXMin > 0. && eventXMax > 0.)
        {
          limits[0] = eventXMin;
          limits[1] = eventXMax;
        }
        else
        {
          limits[0] = Xin.front();
          limits[1] = Xin.back();
        }

        // Rebin the weights - note that this is a distribution
        VectorHelper::rebin(limits,weights_default,emptyVec,Xout,groupWgt,EOutDummy,true,true);
      }
      prog->report();
    } // end of loop for input spectra

    // Calculate the bin widths
    std::vector<double> widths(Xout.size());
    std::adjacent_difference(Xout.begin(),Xout.end(),widths.begin());

    // Take the square root of the errors
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),
                   static_cast<double (*)(double)>(std::sqrt));

    // Multiply the data and errors by the bin widths because the rebin function, when used
    // in the fashion above for the weights, doesn't put it back in
    std::transform(Yout.begin(),Yout.end(),widths.begin()+1,Yout.begin(),std::multiplies<double>());
    std::transform(Eout.begin(),Eout.end(),widths.begin()+1,Eout.begin(),std::multiplies<double>());

    // Now need to normalise the data (and errors) by the weights
    std::transform(Yout.begin(),Yout.end(),groupWgt.begin(),Yout.begin(),std::divides<double>());
    std::transform(Eout.begin(),Eout.end(),groupWgt.begin(),Eout.begin(),std::divides<double>());
    // Now multiply by the number of spectra in the group
    std::transform(Yout.begin(),Yout.end(),Yout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));

    prog->report();
    PARALLEL_END_INTERUPT_REGION
  } // end of loop for groups
  PARALLEL_CHECK_INTERUPT_REGION

  delete prog;

  setProperty("OutputWorkspace",out);

  this->cleanup();
}


//=============================================================================
/** Executes the algorithm in the case of an Event input workspace
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void DiffractionFocussing2::execEvent()
{
  //Create a new outputworkspace with not much in it
  DataObjects::EventWorkspace_sptr out;
  out = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace",1,2,1) );
  //Copy required stuff from it
  API::WorkspaceFactory::Instance().initializeFromParent(m_matrixInputW, out, true);

  MatrixWorkspace_const_sptr outputWS = getProperty("OutputWorkspace");
  bool inPlace = ( m_matrixInputW == outputWS );
  if (inPlace)
    g_log.debug("Focussing EventWorkspace in-place.");
  g_log.debug() << nGroups << " groups found in .cal file (counting group 0).\n";

  EventType eventWtype = m_eventW->getEventType();

  Progress * prog;
  prog = new Progress(this,0.2,0.25,nGroups);

  // determine precount size
  vector<size_t> size_required(this->m_validGroups.size(),0);
  int totalHistProcess = 0;
  for (size_t iGroup = 0; iGroup < this->m_validGroups.size(); iGroup++)
  {
    const int group = this->m_validGroups[iGroup];
    const vector<size_t> &indices = this->m_wsIndices[group];

    totalHistProcess += static_cast<int>(indices.size());
    for (vector<size_t>::const_iterator index = indices.begin();
         index != indices.end(); ++index)
    {
      size_required[iGroup] += m_eventW->getEventList(*index).getNumberEvents();
    }
    prog->report(1, "Pre-counting");
  }

  // ------------- Pre-allocate Event Lists ----------------------------
  delete prog; prog = new Progress(this,0.25,0.3,totalHistProcess);

  // This creates and reserves the space required
  for (size_t iGroup = 0; iGroup < this->m_validGroups.size(); iGroup++)
  {
    const int group = this->m_validGroups[iGroup];
    EventList & groupEL = out->getOrAddEventList(iGroup);
    groupEL.switchTo(eventWtype);
    groupEL.reserve(size_required[iGroup]);
    groupEL.clearDetectorIDs();
    groupEL.setSpectrumNo(group);
    prog->reportIncrement(1, "Allocating");
  }

  // ----------- Focus ---------------
  delete prog; prog = new Progress(this,0.3,0.9,totalHistProcess);

  if (this->m_validGroups.size() == 1)
  {
    g_log.information() << "Performing focussing on a single group\n";
    // Special case of a single group - parallelize differently
    EventList & groupEL = out->getOrAddEventList(0);
    const int group = m_validGroups[0];
    const std::vector<size_t> & indices = this->m_wsIndices[group];

    int chunkSize = 200;

    // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int wiChunk=0;wiChunk<(totalHistProcess/chunkSize)+1;wiChunk++)
    {
      PARALLEL_START_INTERUPT_REGION

      // Perform in chunks for more efficiency
      int max = (wiChunk+1)*chunkSize;
      if (max > totalHistProcess) max = totalHistProcess;

      // precalculate output size
      /* size_t numEventsInChunk(0);
      for (int wi=wiChunk*chunkSize; wi < max; wi++)
      {
        const int group = groupAtWorkspaceIndex[wi];
        if (group == 1)
        {
          // Accumulate the chunk
          numEventsInChunk += eventW->getEventList(wi).getNumberEvents();
        }
      } */

      // Make a blank EventList that will accumulate the chunk.
      EventList chunkEL;
      chunkEL.switchTo(eventWtype);
      //chunkEL.reserve(numEventsInChunk);

      // process the chunk
      for (int i=wiChunk*chunkSize; i < max; i++)
      {
        // Accumulate the chunk
        size_t wi = indices[i];
        chunkEL += m_eventW->getEventList(wi);
      }

      // Rejoin the chunk with the rest.
      PARALLEL_CRITICAL( DiffractionFocussing2_JoinChunks )
      {
        groupEL += chunkEL;
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }
  else
  {
    // ------ PARALLELIZE BY GROUPS -------------------------

    int nValidGroups = static_cast<int>(this->m_validGroups.size());
    PARALLEL_FOR1(m_eventW)
    for (int iGroup = 0; iGroup < nValidGroups; iGroup++)
    {
      PARALLEL_START_INTERUPT_REGION
      const int group = this->m_validGroups[iGroup];
      const std::vector<size_t> & indices = this->m_wsIndices[group];
      for (size_t i=0; i<indices.size(); i++)
      {
        size_t wi = indices[i];

        //In workspace index iGroup, put what was in the OLD workspace index wi
        out->getOrAddEventList(iGroup) += m_eventW->getEventList(wi);

        prog->reportIncrement(1, "Appending Lists");

        // When focussing in place, you can clear out old memory from the input one!
        if (inPlace)
        {
          boost::const_pointer_cast<EventWorkspace>(m_eventW)->getEventList(wi).clear();
          Mantid::API::MemoryManager::Instance().releaseFreeMemory();
        }
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } // (done with parallel by groups)

  //Now that the data is cleaned up, go through it and set the X vectors to the input workspace we first talked about.
  delete prog; prog = new Progress(this,0.9,1.0,nGroups);
  for (size_t workspaceIndex = 0; workspaceIndex < this->m_validGroups.size(); workspaceIndex++)
  {
    const int group = this->m_validGroups[workspaceIndex];
    //Now this is the workspace index of that group; simply 1 offset
    prog->reportIncrement(1, "Setting X");

    if (workspaceIndex >= out->getNumberHistograms())
    {
      g_log.warning() << "Warning! Invalid workspace index found for group # " << group << ". Histogram will be empty.\n";
      continue;
    }

    //Now you set the X axis to the X you saved before.
    if (group2xvector.size() > 0)
    {
      group2vectormap::iterator git = group2xvector.find(group);
      if (git != group2xvector.end())
        out->setX(workspaceIndex, (git->second) );
      else
        //Just use the 1st X vector it found, instead of nothin.
        out->setX(workspaceIndex, (group2xvector.begin()->second) );
    }
    else
      g_log.warning() << "Warning! No X histogram bins were found for any groups. Histogram will be empty.\n";
  }
  out->clearMRU();
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(out));
  delete prog;
  return;
}


//=============================================================================
/** Verify that all the contributing detectors to a spectrum belongs to the same group
 *  @param wi :: The workspace index in the workspace
 *  @return Group number if successful otherwise return -1
 */
int DiffractionFocussing2::validateSpectrumInGroup(size_t wi)
{
  const std::set<detid_t> & dets = m_matrixInputW->getSpectrum(wi)->getDetectorIDs();
  if (dets.empty()) // Not in group
  {
    g_log.debug() << wi << " <- this workspace index is empty!\n";
    return -1;
  }

  std::set<detid_t>::const_iterator it = dets.begin();
  if (*it < 0) // bad pixel id
    return -1;

  try
  {// what if index out of range?
    const int group = udet2group.at(*it);
    if (group <= 0)
      return -1;
    it++;
    for (; it != dets.end(); ++it) // Loop other all other udets
    {
      if (udet2group.at(*it) != group)
        return -1;
    }
    return group;
  }
  catch(...)
  {}

  return -1;
}




//=============================================================================
/** Determine the rebinning parameters, i.e Xmin, Xmax and logarithmic step for each group
 * Looks for the widest range of X bins (lowest min and highest max) of
 *  all the spectra in a group and sets the output group X bin boundaries to use
 *  those limits.
 *  The X histogram is set to log binning with the same # of points between max and min
 *  as the input spectra.
 *
 * The X vectors are saved in group2xvector.
 * It also initializes the groupAtWorkspaceIndex[] array.
 *
 */
void DiffractionFocussing2::determineRebinParameters()
{
  std::ostringstream mess;

  // typedef for the storage of the group ranges
  typedef std::map<int, std::pair<double, double> > group2minmaxmap;
  // Map from group number to its associated range parameters <Xmin,Xmax,step>
  group2minmaxmap group2minmax;
  group2minmaxmap::iterator gpit;

  const double BIGGEST = std::numeric_limits<double>::max();

  // whether or not to bother checking for a mask
  bool checkForMask = false;
  Geometry::Instrument_const_sptr instrument = m_matrixInputW->getInstrument();
  if (instrument != NULL)
  {
    checkForMask = ((instrument->getSource() != NULL) && (instrument->getSample() != NULL));
  }

  groupAtWorkspaceIndex.resize(nHist);
  for (int wi = 0; wi < nHist; wi++) //  Iterate over all histograms to find X boundaries for each group
  {
    const int group = validateSpectrumInGroup(static_cast<size_t>(wi));
    groupAtWorkspaceIndex[wi] = group;
    if (group == -1)
      continue;

    // the spectrum is the real thing we want to work with
    const ISpectrum * spec = m_matrixInputW->getSpectrum(wi);
    if (spec == NULL)
    {
      groupAtWorkspaceIndex[wi] = -1;
      continue;
    }
    if (checkForMask)
    {
      if (instrument->isDetectorMasked(spec->getDetectorIDs()))
      {
        groupAtWorkspaceIndex[wi] = -1;
        continue;
      }
    }
    gpit = group2minmax.find(group);

    // Create the group range in the map if it isn't already there
    if (gpit == group2minmax.end())
    {
      gpit = group2minmax.insert(std::make_pair(group,std::make_pair(BIGGEST,-1.*BIGGEST))).first;
    }
    const double min = (gpit->second).first;
    const double max = (gpit->second).second;
    const MantidVec& X = spec->readX();
    double temp = X.front();
    if (temp < (min)) //New Xmin found
      (gpit->second).first = temp;
    temp = X.back();
    if (temp > (max)) //New Xmax found
      (gpit->second).second = temp;
  }

  nGroups=group2minmax.size(); // Number of unique groups

  double Xmin, Xmax, step;
  const int64_t xPoints = nPoints + 1;

  //Iterator over all groups to create the new X vectors
  for (gpit = group2minmax.begin(); gpit != group2minmax.end(); gpit++)
  {
    Xmin = (gpit->second).first;
    Xmax = (gpit->second).second;

    //Make sure that Xmin is not 0 - since it is not possible to do log binning from 0.0.
    if (Xmin <= 0) Xmin = Xmax / nPoints;
    if (Xmin <= 0) Xmin = 1.0;

    if (Xmax < Xmin) // Should never happen
    {
      mess << "Fail to determine X boundaries for group:" << gpit->first << "\n";
      mess << "The boundaries are (Xmin,Xmax):" << Xmin << " " << Xmax;
      throw std::runtime_error(mess.str());
    }
    //This log step size will give the right # of points
    step = (log(Xmax) - log(Xmin)) / nPoints;
    mess << "Found Group:" << gpit->first << "(Xmin,Xmax,log step):" << (gpit->second).first
        << "," << (gpit->second).second << "," << step;
    //g_log.information(mess.str());
    mess.str("");

    //Build up the X vector.
    boost::shared_ptr<MantidVec> xnew = boost::shared_ptr<MantidVec>(new MantidVec(xPoints)); //New X vector
    (*xnew)[0] = Xmin;
    for (int64_t j = 1; j < xPoints; j++)
    {
      (*xnew)[j] = Xmin * (1.0 + step);
      Xmin = (*xnew)[j];
    }
    group2xvector[gpit->first] = xnew; //Register this vector in the map
  }
  // Not needed anymore
  udet2group.clear();
  return;
}

/***
 * Configure the mapping of output group to list of input workspace
 * indices, and the list of valid group numbers.
 *
 * @return the total number of input histograms that will be read.
 */
size_t DiffractionFocussing2::setupGroupToWSIndices()
{
  // set up the mapping of group to input workspace index
  this->m_wsIndices.reserve(this->nGroups+1);
  size_t nHist_st = static_cast<size_t>(nHist);
  for (size_t wi=0;wi<nHist_st;wi++)
  {
    //wi is the workspace index (of the input)
    const int group = groupAtWorkspaceIndex[wi];
    if (group < 1) // Not in a group, or invalid group #
      continue;

    // resize the ws_indices if it is not big enough
    if (this->m_wsIndices.size() < static_cast<size_t>(group+1))
    {
      this->m_wsIndices.resize(group + 1);
    }

    // Also record a list of workspace indices
    this->m_wsIndices[group].push_back(wi);
  }

  // initialize a vector of the valid group numbers
  this->m_validGroups.reserve(nGroups);
  size_t totalHistProcess = 0;
  for (size_t i = 0; i <this->m_wsIndices.size(); i++)
  {
    if (!(this->m_wsIndices[i].empty()))
    {
      this->m_validGroups.push_back(static_cast<int>(i));
      totalHistProcess += this->m_wsIndices[i].size();
    }
  }

  return totalHistProcess;
}

} // namespace Algorithm
} // namespace Mantid
