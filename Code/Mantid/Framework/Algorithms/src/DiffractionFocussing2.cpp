//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"
#include <cfloat>
#include <fstream>
#include "MantidKernel/VectorHelper.h"
#include <iterator>
#include <numeric>

namespace Mantid
{
using namespace Kernel;
using namespace API;

namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)

using DataObjects::Workspace2D;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

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
  this->setOptionalMessage(
      "Sums (focusses) spectra together to make one spectrum per group.\n"
      "The groups are specifed in a grouping file."
      );

  API::CompositeValidator<MatrixWorkspace> *wsValidator = new API::CompositeValidator<MatrixWorkspace>;
  wsValidator->add(new API::WorkspaceUnitValidator<MatrixWorkspace>("dSpacing"));
  wsValidator->add(new API::RawCountValidator<MatrixWorkspace>);
  declareProperty(
    new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A 2D workspace with X values of d-spacing" );
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );

  declareProperty(new FileProperty("GroupingFileName", "", FileProperty::Load, ".cal"),
		  "The name of the CalFile with grouping data" );
  declareProperty("MatrixWorkspaceOut", false, "Force the output workspace to be a Matrix workspace");
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

  readGroupingFile(groupingFileName);

  // Get the input workspace
  matrixInputW = getProperty("InputWorkspace");
  nPoints = matrixInputW->blocksize();
  nHist = matrixInputW->getNumberHistograms();

  //This finds the rebin parameters (used in both versions)
  // It also initializes the groupAtWorkspaceIndex[] array.
  determineRebinParameters();

  eventW = boost::dynamic_pointer_cast<EventWorkspace>( matrixInputW );
  if ((!getProperty("MatrixWorkspaceOut")) && (eventW != NULL))
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
  
  API::Progress progress(this,0.0,1.0,nHist+nGroups);
  for (int i=0;i<nHist;i++)
  {
    progress.report();

    //Check whether this spectra is in a valid group
    const int group=groupAtWorkspaceIndex[i];
    if (group==-1) // Not in a group
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
      out->dataX(static_cast<int>(dif))=Xout;
      flags[dif]=false;
      // Initialize the group's weight vector here too
      group2wgtvector[group] = boost::shared_ptr<MantidVec>(new MantidVec(nPoints,0.0));
      // Also set the spectrum number to the group number
      out->getAxis(1)->spectraNo(static_cast<int>(dif)) = group;
    }
    // Add the detectors for this spectrum to the output workspace's spectra-detector map
    out->mutableSpectraMap().addSpectrumEntries(group,inSpecMap.getDetectors(inSpecAxis->spectraNo(i)));
    // Get the references to Y and E output and rebin
    MantidVec& Yout=out->dataY(static_cast<int>(dif));
    MantidVec& Eout=out->dataE(static_cast<int>(dif));
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

  for (int i=0; i < nGroups; ++i,++wit)
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

    progress.report();
  }
  
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
  API::WorkspaceFactory::Instance().initializeFromParent(matrixInputW, out, true);

  bool inPlace = (this->getPropertyValue("InputWorkspace") == this->getPropertyValue("OutputWorkspace"));
  if (inPlace)
    g_log.information("Focussing EventWorkspace in-place.");

  //BUT! We want to use all groups, even if no pixels ever refer to them.
  nGroups = maxgroup_in_file+1;
  //g_log.information() << nGroups << " groups found in .cal file (counting group 0).\n";

  //Flag to determine whether the X for a group has been set
  std::vector<bool> flags(nGroups,true);

  // ------------- Pre-allocate Event Lists ----------------------------
  std::vector<size_t> size_required(nGroups,0);
  for (int i=0;i<nHist;i++)
  {
    //i is the workspace index (of the input)
    //Check whether this spectra is in a valid group
    const int group = groupAtWorkspaceIndex[i];
    if (group < 1) // Not in a group
      continue;
    size_required[group] += eventW->getEventList(i).getNumberEvents();
  }
  for (int group=1; group<nGroups; group++)
    out->getOrAddEventList(group-1).reserve(size_required[group]);


  API::Progress progress(this,0.0,1.0,nHist+nGroups);
  for (int i=0;i<nHist;i++)
  {
    progress.report();
    //i is the workspace index (of the input)

    //Check whether this spectra is in a valid group
    const int group = groupAtWorkspaceIndex[i];
    if (group < 1) // Not in a group
      continue;

    // Assign the new X axis only once (i.e when this group is encountered the first time)
    if (flags[group])
    {
      //Copy the X axis for later
      //original_X_to_use[group].assign( eventW->refX(i)->begin(), eventW->refX(i)->end())  ;
      //Flag not to copy it again.
      flags[group]=false;
    }

    //In workspace index group-1, put what was in the OLD workspace index i
    out->getOrAddEventList(group-1) += eventW->getEventList(i);

    // When focussing in place, you can clear out old memory from the input one!
    if (inPlace)
      eventW->getEventList(i).clear();
  }

  //Now, we want to make sure that all groups are listed in the output workspace,
  //  even those with no events at all
  for (int group=1; group < this->nGroups; group++)
  {
    //Flags still true = this group was not touched yet
    if (flags[group])
    {
      //Simply getting the event list will create it, if not already there.
      EventList& emptyEventList = out->getOrAddEventList(group-1);
      emptyEventList.clear();
      //If the X axis hasn't been set, just use the one from workspace index 0.
      //original_X_to_use[group].assign( eventW->refX(0)->begin(), eventW->refX(0)->end())  ;
    }
  }

  //Finalize the maps
  out->doneAddingEventLists();

  //Now that the data is cleaned up, go through it and set the X vectors to the input workspace we first talked about.

  for (int g=1; g < nGroups; g++)
  {
    //Now this is the workspace index of that group; simply 1 offset
    int workspaceIndex = g-1;

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
int DiffractionFocussing2::validateSpectrumInGroup(int spectrum_number)
{
  // Get the spectra to detector map
  const API::SpectraDetectorMap& spectramap = matrixInputW->spectraMap();
  const std::vector<int> dets = spectramap.getDetectors(spectrum_number);
  if (dets.empty()) // Not in group
  {
    std::cout << spectrum_number << " <- this spectrum is empty!\n";
    return -1;
  }

  std::vector<int>::const_iterator it = dets.begin();
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
/// Reads in the file with the grouping information
/// @param groupingFileName :: The file that contains the group information
///
void DiffractionFocussing2::readGroupingFile(const std::string& groupingFileName)
{
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile.is_open())
  {
    g_log.error() << "Unable to open grouping file " << groupingFileName << std::endl;
    throw Exception::FileError("Error reading .cal file",groupingFileName);
  }
  maxgroup_in_file = 0;

  udet2group.clear();
  std::string str;
  while(getline(grFile,str))
  {
    //Comment
    if (str.empty() || str[0] == '#') continue;
    std::istringstream istr(str);
    int n,udet,sel,group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    if ((sel) && (group>0))
    {
      udet2group[udet]=group; //Register this udet
      //Track the highest group #
      if (group > maxgroup_in_file)
        maxgroup_in_file = group;
    }
  }
  grFile.close();

  return;
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

  groupAtWorkspaceIndex.resize(nHist);
  const API::Axis* const spectra_Axis = matrixInputW->getAxis(1);

  for (int i = 0; i < nHist; i++) //  Iterate over all histograms to find X boundaries for each group
  {
    const int group = validateSpectrumInGroup(spectra_Axis->spectraNo(i));
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
  const int xPoints = nPoints + 1;

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
    for (int j = 1; j < xPoints; j++)
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
