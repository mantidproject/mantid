//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/VectorHelper.h"
#include <cfloat>
#include <fstream>
#include <iterator>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{

namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)

/// Sets documentation strings for this algorithm
void DiffractionFocussing2::initDocs()
{
  this->setWikiSummary("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a [[CalFile]]. ");
  this->setOptionalMessage("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a CalFile.");
}



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

  API::CompositeValidator<MatrixWorkspace> *wsValidator = new API::CompositeValidator<MatrixWorkspace>;
  wsValidator->add(new API::WorkspaceUnitValidator<MatrixWorkspace>("dSpacing"));
  wsValidator->add(new API::RawCountValidator<MatrixWorkspace>);
  declareProperty(
    new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A 2D workspace with X values of d-spacing" );
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );

  declareProperty(new FileProperty("GroupingFileName", "", FileProperty::OptionalLoad, ".cal"),
      "Optional: The name of the CalFile with grouping data." );

  declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace", "", Direction::Input, true),
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
  group2xvector.clear();
  group2wgtvector.clear();
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
  matrixInputW = getProperty("InputWorkspace");
  nPoints = matrixInputW->blocksize();
  nHist = matrixInputW->getNumberHistograms();

  // --- Do we need to read the grouping workspace? ----
  if (groupingFileName != "")
  {
    progress(0.01, "Reading grouping file");
    IAlgorithm_sptr childAlg = createSubAlgorithm("CreateGroupingWorkspace");
    childAlg->setProperty("InputWorkspace", matrixInputW);
    childAlg->setProperty("OldCalFilename", groupingFileName);
    childAlg->executeAsSubAlg();
    groupWS = childAlg->getProperty("OutputWorkspace");
  }

  // Fill the map
  progress(0.2, "Determine Rebin Params");
  udet2group.clear();
  groupWS->makeDetectorIDToGroupMap(udet2group, nGroups);
  //std::cout << "nGroups " << nGroups << "\n";

  //This finds the rebin parameters (used in both versions)
  // It also initializes the groupAtWorkspaceIndex[] array.
  determineRebinParameters();

  eventW = boost::dynamic_pointer_cast<EventWorkspace>( matrixInputW );
  if ((getProperty("PreserveEvents")) && (eventW != NULL))
  {
    //Input workspace is an event workspace. Use the other exec method
    this->execEvent();
    this->cleanup();
    return;
  }

  //No problem? Then it is a normal Workspace2D
  API::MatrixWorkspace_sptr out=API::WorkspaceFactory::Instance().create(matrixInputW,nGroups,nPoints+1,nPoints);

  // The spectaDetectorMap will have been copied from the input, but we don't want it
  out->mutableSpectraMap().clear();

  // Now the real work
  std::vector<bool> flags(nGroups,true); //Flag to determine whether the X for a group has been set
  MantidVec limits(2), weights_default(1,1.0), emptyVec(1,0.0), EOutDummy(nPoints);  // Vectors for use with the masking stuff

  const API::SpectraDetectorMap& inSpecMap = matrixInputW->spectraMap();
  const API::Axis* const inSpecAxis = matrixInputW->getAxis(1);
  
  Progress * prog;
  prog = new API::Progress(this,0.2,1.0,nHist+nGroups);
  for (int64_t i=0;i<nHist;i++)
  {
    prog->report();

    //Check whether this spectra is in a valid group
    const int group=groupAtWorkspaceIndex[i];
    //std::cout << "Wi " << i << " is at group " << group << "\n";
    if (group<=0) // Not in a group
      continue;
    //Get reference to its old X,Y,and E.
    const MantidVec& Xin=matrixInputW->readX(i);
    const MantidVec& Yin=matrixInputW->readY(i);
    const MantidVec& Ein=matrixInputW->readE(i);
    // Get the group
    group2vectormap::iterator it=group2xvector.find(group);
    group2vectormap::difference_type dif=std::distance(group2xvector.begin(),it);
    const MantidVec& Xout = *((*it).second);
    // Assign the new X axis only once (i.e when this group is encountered the first time)
    if (flags[dif])
    {
      out->dataX(static_cast<int64_t>(dif))=Xout;
      flags[dif]=false;
      // Initialize the group's weight vector here too
      group2wgtvector[group] = boost::shared_ptr<MantidVec>(new MantidVec(nPoints,0.0));
      // Also set the spectrum number to the group number
      out->getAxis(1)->spectraNo(static_cast<int64_t>(dif)) = group;
    }
    // Add the detectors for this spectrum to the output workspace's spectra-detector map
    out->mutableSpectraMap().addSpectrumEntries(group,inSpecMap.getDetectors(inSpecAxis->spectraNo(i)));
    // Get the references to Y and E output and rebin
    MantidVec& Yout=out->dataY(static_cast<int64_t>(dif));
    MantidVec& Eout=out->dataE(static_cast<int64_t>(dif));
    try
    {
      VectorHelper::rebinHistogram(Xin,Yin,Ein,Xout,Yout,Eout,true);
    }catch(...)
    {
      // Should never happen because Xout is constructed to envelop all of the Xin vectors
      std::ostringstream mess;
      mess << "Error in rebinning process for spectrum:" << i;
      throw std::runtime_error(mess.str());
    }
    
    // Get a reference to the summed weights vector for this group
    MantidVec& groupWgt = *group2wgtvector[group];
    // Check for masked bins in this spectrum
    if ( matrixInputW->hasMaskedBins(i) )
    {
      MantidVec weight_bins,weights;
      weight_bins.push_back(Xin.front());
      // If there are masked bins, get a reference to the list of them
      const API::MatrixWorkspace::MaskList& mask = matrixInputW->maskedBins(i);
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
      
      // Create a temporary vector for the rebinned angles
      //MantidVec weightsTemp(groupWgt.size(),0.0);
      //MantidVec zeroesTemp(groupWgt.size(),0.0);
      // Create a zero vector for the errors because we don't care about them here
      const MantidVec zeroes(weights.size(),0.0);
      // Rebin the weights - note that this is a distribution
      //VectorHelper::rebin(weight_bins,weights,zeroes,Xout,weightsTemp,zeroesTemp,true);
      VectorHelper::rebin(weight_bins,weights,zeroes,Xout,groupWgt,EOutDummy,true,true);
      // Add weights for this spectrum to the output weights vector
      //std::transform(groupWgt.begin(),groupWgt.end(),weightsTemp.begin(),groupWgt.begin(),std::plus<double>());
    }
    else // If no masked bins we want to add 1 to the weight of the output bins that this input covers
    {
      limits[0] = Xin.front();
      limits[1] = Xin.back();
      // Rebin the weights - note that this is a distribution
      VectorHelper::rebin(limits,weights_default,emptyVec,Xout,groupWgt,EOutDummy,true,true);
    }
  }
  
  // Now propagate the errors.
  // Pointer to sqrt function
  typedef double (*uf)(double);
  uf rs=std::sqrt;
  
  group2vectormap::const_iterator wit = group2wgtvector.begin();

  for (int64_t i=0; i < nGroups; ++i,++wit)
  {
    const MantidVec& Xout = out->readX(i);
    // Calculate the bin widths
    std::vector<double> widths(Xout.size());
    std::adjacent_difference(Xout.begin(),Xout.end(),widths.begin());

    MantidVec& Yout=out->dataY(i);
    MantidVec& Eout=out->dataE(i);
    // Take the square root of the errors
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),rs);

    // Multiply the data and errors by the bin widths because the rebin function, when used
    // in the fashion above for the weights, doesn't put it back in
    std::transform(Yout.begin(),Yout.end(),widths.begin()+1,Yout.begin(),std::multiplies<double>());
    std::transform(Eout.begin(),Eout.end(),widths.begin()+1,Eout.begin(),std::multiplies<double>());

    // Now need to normalise the data (and errors) by the weights
    const MantidVec& wgt = *(*wit).second;
    std::transform(Yout.begin(),Yout.end(),wgt.begin(),Yout.begin(),std::divides<double>());
    std::transform(Eout.begin(),Eout.end(),wgt.begin(),Eout.begin(),std::divides<double>());
    // Now multiply by the number of spectra in the group
    const int groupSize = std::count(groupAtWorkspaceIndex.begin(),groupAtWorkspaceIndex.end(),(*wit).first);
    std::transform(Yout.begin(),Yout.end(),Yout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));

    prog->report();
  }
  
  setProperty("OutputWorkspace",out);

  delete prog;
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
  API::WorkspaceFactory::Instance().initializeFromParent(matrixInputW, out, true);

  bool inPlace = (this->getPropertyValue("InputWorkspace") == this->getPropertyValue("OutputWorkspace"));
  if (inPlace)
    g_log.debug("Focussing EventWorkspace in-place.");
  g_log.debug() << nGroups << " groups found in .cal file (counting group 0).\n";

  Progress * prog;
  prog = new Progress(this,0.2,0.35,nHist);

  if (nGroups == 1)
  {
    g_log.information() << "Performing focussing on a single group\n";
    // Special case of a single group - parallelize differently
    EventList & groupEL = out->getOrAddEventList(0);

    int chunkSize = 200;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) if (eventW->threadSafe()) )
    for (int wiChunk=0;wiChunk<nHist/chunkSize;wiChunk++)
    {
      PARALLEL_START_INTERUPT_REGION

      // Make a blank EventList that will accumulate the chunk.
      EventList chunkEL;

      // Perform in chunks for more efficiency
      int max = (wiChunk+1)*chunkSize;
      if (max > nHist) max = nHist;
      for (int wi=wiChunk*chunkSize; wi < max; wi++)
      {
        const int group = groupAtWorkspaceIndex[wi];
        if (group == 1)
        {
          // Accumulate the chunk
          chunkEL += eventW->getEventList(wi);
        }
      }

      // Rejoin the chunk with the rest.
      PARALLEL_CRITICAL( DiffractionFocussing2_JoinChunks )
      {
        groupEL += chunkEL;
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    out->doneAddingEventLists();
  }
  else
  {
    // ------ PARALLELIZE BY GROUPS -------------------------
    // ------------- Pre-allocate Event Lists ----------------------------
    std::vector< std::vector<int> > ws_indices(nGroups+1);
    std::vector<size_t> size_required(nGroups+1,0);
    Geometry::IInstrument_const_sptr instrument = eventW->getInstrument();
    Geometry::IObjComponent_const_sptr source;
    Geometry::IObjComponent_const_sptr sample;
    if (instrument != NULL)
    {
      source = instrument->getSource();
      sample = instrument->getSample();
    }

    for (int wi=0;wi<nHist;wi++)
    {
      if (instrument != NULL)
      {
        if ( source != NULL && sample != NULL )
        {
          Geometry::IDetector_const_sptr det = eventW->getDetector(static_cast<size_t>(wi));
          if ( det->isMasked() ) continue;
        }
      }
      //i is the workspace index (of the input)
      const int group = groupAtWorkspaceIndex[wi];
      if (group < 1) // Not in a group
        continue;
      size_required[group] += eventW->getEventList(wi).getNumberEvents();
      // Also record a list of workspace indices
      ws_indices[group].push_back(wi);
      prog->reportIncrement(1, "Pre-counting");
    }

    delete prog; prog = new Progress(this,0.15,0.3,nGroups);
    // This creates and reserves the space required
    for (int group=1; group<nGroups+1; group++)
    {
      out->getOrAddEventList(group-1).reserve(size_required[group]);
      prog->reportIncrement(1, "Allocating");
    }

    // ----------- Focus ---------------
    delete prog; prog = new Progress(this,0.40,0.9,nHist);
    PARALLEL_FOR1(eventW)
    for (int group=1; group<nGroups+1; group++)
    {
      PARALLEL_START_INTERUPT_REGION
      std::vector<int> indices = ws_indices[group];
      for (size_t i=0; i<indices.size(); i++)
      {
        int wi = indices[i];

        //In workspace index group-1, put what was in the OLD workspace index wi
        out->getOrAddEventList(group-1) += eventW->getEventList(wi);
        prog->reportIncrement(1, "Appending Lists");

        // When focussing in place, you can clear out old memory from the input one!
        if (inPlace)
        {
          eventW->getEventList(wi).clear();
          Mantid::API::MemoryManager::Instance().releaseFreeMemory();
        }
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    //Finalize the maps
    out->doneAddingEventLists();

  } // (done with parallel by groups)

  //Now that the data is cleaned up, go through it and set the X vectors to the input workspace we first talked about.
  delete prog; prog = new Progress(this,0.9,1.0,nGroups);
  for (int g=1; g < nGroups+1; g++)
  {
    //Now this is the workspace index of that group; simply 1 offset
    int workspaceIndex = g-1;
    prog->reportIncrement(1, "Setting X");

    if (workspaceIndex >= out->getNumberHistograms())
    {
      g_log.warning() << "Warning! Invalid workspace index found for group # " << g << ". Histogram will be empty.\n";
      continue;
    }

    //Now you set the X axis to the X you saved before.
    if (group2xvector.size() > 0)
    {
      group2vectormap::iterator git = group2xvector.find(g);
      if (git != group2xvector.end())
        out->setX(workspaceIndex, *(git->second) );
      else
        //Just use the 1st X vector it found, instead of nothin.
        out->setX(workspaceIndex, *(group2xvector.begin()->second) );
    }
    else
      g_log.warning() << "Warning! No X histogram bins were found for any groups. Histogram will be empty.\n";
  }
  out->clearMRU();
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(out));
  return;
}


//=============================================================================
/** Verify that all the contributing detectors to a spectrum belongs to the same group
 *  @param spectrum_number :: The spectrum number in the workspace
 *  @return Group number if successful otherwise return -1
 */
int DiffractionFocussing2::validateSpectrumInGroup(specid_t spectrum_number)
{
  // Get the spectra to detector map
  const API::SpectraDetectorMap& spectramap = matrixInputW->spectraMap();
  const std::vector<detid_t> dets = spectramap.getDetectors(spectrum_number);
  if (dets.empty()) // Not in group
  {
    std::cout << spectrum_number << " <- this spectrum is empty!\n";
    return -1;
  }

  std::vector<detid_t>::const_iterator it = dets.begin();
  udet2groupmap::const_iterator mapit = udet2group.find((*it)); //Find the first udet
  if (mapit == udet2group.end()) // The first udet that contributes to this spectra is not assigned to a group
    return -1;
  const int group = (*mapit).second;
  int new_group;
  for (it + 1; it != dets.end(); it++) // Loop other all other udets
  {
    mapit = udet2group.find((*it));
    if (mapit == udet2group.end()) // Group not assigned
      return -1;
    new_group = (*mapit).second;
    if (new_group != group) // At least one udet does not belong to the same group
      return -1;
  }

  return group;
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
  typedef std::map<int64_t, std::pair<double, double> > group2minmaxmap;
  // Map from group number to its associated range parameters <Xmin,Xmax,step>
  group2minmaxmap group2minmax;
  group2minmaxmap::iterator gpit;

  groupAtWorkspaceIndex.resize(nHist);
  const API::Axis* const spectra_Axis = matrixInputW->getAxis(1);

  for (int64_t i = 0; i < nHist; i++) //  Iterate over all histograms to find X boundaries for each group
  {
    const int64_t group = validateSpectrumInGroup(spectra_Axis->spectraNo(i));
    groupAtWorkspaceIndex[i] = group;
    if (group == -1)
      continue;
    gpit = group2minmax.find(group);
    // Create the group range in the map if it isn't already there
    if (gpit == group2minmax.end())
    {
      gpit = group2minmax.insert(std::make_pair(group,std::make_pair(1.0e14,-1.0e14))).first;
    }
    const double min = ((*gpit).second).first;
    const double max = ((*gpit).second).second;
    const MantidVec& X = matrixInputW->readX(i);
    if (X.front() < (min)) //New Xmin found
      ((*gpit).second).first = X.front();
    if (X.back() > (max)) //New Xmax found
      ((*gpit).second).second = X.back();
  }

  nGroups=group2minmax.size(); // Number of unique groups

  double Xmin, Xmax, step;
  const int64_t xPoints = nPoints + 1;

  //Iterator over all groups to create the new X vectors
  for (gpit = group2minmax.begin(); gpit != group2minmax.end(); gpit++)
  {
    Xmin = ((*gpit).second).first;
    Xmax = ((*gpit).second).second;

    //Make sure that Xmin is not 0 - since it is not possible to do log binning from 0.0.
    if (Xmin <= 0) Xmin = Xmax / nPoints;
    if (Xmin <= 0) Xmin = 1.0;

    if (Xmax < Xmin) // Should never happen
    {
      mess << "Fail to determine X boundaries for group:" << (*gpit).first << "\n";
      mess << "The boundaries are (Xmin,Xmax):" << Xmin << " " << Xmax;
      throw std::runtime_error(mess.str());
    }
    //This log step size will give the right # of points
    step = (log(Xmax) - log(Xmin)) / nPoints;
    mess << "Found Group:" << ((*gpit).first) << "(Xmin,Xmax,log step):" << ((*gpit).second).first
        << "," << ((*gpit).second).second << "," << step;
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
    group2xvector[(*gpit).first] = xnew; //Register this vector in the map
  }
  // Not needed anymore
  udet2group.clear();
  return;
}


} // namespace Algorithm
} // namespace Mantid
