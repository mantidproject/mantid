//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cfloat>
#include <fstream>
#include "MantidKernel/VectorHelper.h"
#include <iterator>
#include <numeric>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)

using namespace Kernel;
using DataObjects::Workspace2D;

/// Constructor
DiffractionFocussing2::DiffractionFocussing2() : 
  API::Algorithm(), inputW(), udet2group(), spectra_group(), group2xvector(),
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
  API::CompositeValidator<Workspace2D> *wsValidator = new API::CompositeValidator<Workspace2D>;
  wsValidator->add(new API::WorkspaceUnitValidator<Workspace2D>("dSpacing"));
  wsValidator->add(new API::RawCountValidator<Workspace2D>);
  declareProperty(
    new API::WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,wsValidator),
    "A 2D workspace with X values of d-spacing" );
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );

  declareProperty(new FileProperty("GroupingFileName", "", FileProperty::Load, std::vector<std::string>(1,"cal")),
		  "The name of the CalFile with grouping data" );
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void DiffractionFocussing2::exec()
{
  // retrieve the properties
  std::string groupingFileName=getProperty("GroupingFileName");

  // Get the input workspace
  inputW = getProperty("InputWorkspace");
  nPoints=inputW->readX(0).size();
  nHist=inputW->getNumberHistograms();

  readGroupingFile(groupingFileName);

  determineRebinParameters();

  API::MatrixWorkspace_sptr out=API::WorkspaceFactory::Instance().create(inputW,nGroups,nPoints,nPoints-1);
  // The spectaDetectorMap will have been copied from the input, but we don't want it
  out->mutableSpectraMap().clear();

  // Now the real work
  std::vector<bool> flags(nGroups,true); //Flag to determine whether the X for a group has been set
  MantidVec limits(2), weights_default(1,1.0), emptyVec(1,0.0);  // Vectors for use with the masking stuff

  const API::SpectraDetectorMap& inSpecMap = inputW->spectraMap();
  const API::Axis* const inSpecAxis = inputW->getAxis(1);
  
  for (int i=0;i<nHist;i++)
  {
    //Check whether this spectra is in a valid group
    const int group=spectra_group[i];
    if (group==-1) // Not in a group
      continue;
    //Get reference to its old X,Y,and E.
    const MantidVec& Xin=inputW->readX(i);
    const MantidVec& Yin=inputW->readY(i);
    const MantidVec& Ein=inputW->readE(i);
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
      group2wgtvector[group] = boost::shared_ptr<MantidVec>(new MantidVec(nPoints-1,0.0));
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
    if ( inputW->hasMaskedBins(i) )
    {
      MantidVec weight_bins,weights;
      weight_bins.push_back(Xin.front());
      // If there are masked bins, get a reference to the list of them
      const API::MatrixWorkspace::MaskList& mask = inputW->maskedBins(i);
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
      // Add weights for this spectrum to the output weights vector
      VectorHelper::rebinNonDispersive(weight_bins,weights,zeroes,Xout,groupWgt,groupWgt,true);
    }
    else // If no masked bins we want to add 1 to the weight of the output bins that this input covers
    {
      limits[0] = Xin.front();
      limits[1] = Xin.back();
      VectorHelper::rebinNonDispersive(limits,weights_default,emptyVec,Xout,groupWgt,groupWgt,true);
    }
        
    progress(static_cast<double>(i)/101.0);
  }
  
  // Now propagate the errors.
  // Pointer to sqrt function
  typedef double (*uf)(double);
  uf rs=std::sqrt;
  
  group2vectormap::const_iterator wit = group2wgtvector.begin();
  
  for (int i=0; i < nGroups; ++i,++wit)
  {
    MantidVec& Eout=out->dataE(i);
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),rs);

    // Now need to normalise the data (and errors) by the weights
    MantidVec& Yout=out->dataY(i);
    const MantidVec& wgt = *(*wit).second;
    std::transform(Yout.begin(),Yout.end(),wgt.begin(),Yout.begin(),std::divides<double>());
    std::transform(Eout.begin(),Eout.end(),wgt.begin(),Eout.begin(),std::divides<double>());
    // Now multiply by the number of spectra in the group
    const int groupSize = std::count(spectra_group.begin(),spectra_group.end(),(*wit).first);
    std::transform(Yout.begin(),Yout.end(),Yout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));
    std::transform(Eout.begin(),Eout.end(),Eout.begin(),std::bind2nd(std::multiplies<double>(),groupSize));
  }
  
  progress(1.0);
  //Do some cleanup
  spectra_group.clear();
  group2xvector.clear();

  setProperty("OutputWorkspace",out);
  return;
}

/// Reads in the file with the grouping information
/// @param groupingFileName The file that contains the group information
///
void DiffractionFocussing2::readGroupingFile(const std::string& groupingFileName)
{
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile.is_open())
  {
    g_log.error() << "Unable to open grouping file " << groupingFileName << std::endl;
    throw Exception::FileError("Error reading .cal file",groupingFileName);
  }

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
    }
  }
  grFile.close();

  return;
}

/// Determine the rebinning parameters, i.e Xmin, Xmax and logarithmic step for each group
void DiffractionFocussing2::determineRebinParameters()
{
  std::ostringstream mess;

  // typedef for the storage of the group ranges
  typedef std::map<int, std::pair<double, double> > group2minmaxmap;
  // Map from group number to its associated range parameters <Xmin,Xmax,step>
  group2minmaxmap group2minmax;
  group2minmaxmap::iterator gpit;

  spectra_group.resize(nHist);
  const API::Axis* const spectra_Axis = inputW->getAxis(1);

  for (int i = 0; i < nHist; i++) //  Iterate over all histograms to find X boundaries for each group
  {
    const int group = validateSpectrumInGroup(spectra_Axis->spectraNo(i));
    spectra_group[i] = group;
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
    const MantidVec& X = inputW->readX(i);
    if (X.front() < (min)) //New Xmin found
      ((*gpit).second).first = X.front();
    if (X.back() > (max)) //New Xmax found
      ((*gpit).second).second = X.back();
  }

  nGroups=group2minmax.size(); // Number of unique groups

  double Xmin, Xmax, step;

  //Iterator over all groups to create the new X vectors
  for (gpit = group2minmax.begin(); gpit != group2minmax.end(); gpit++)
  {
    Xmin = ((*gpit).second).first;
    Xmax = ((*gpit).second).second;
    if (Xmax < Xmin) // Should never happen
    {
      mess << "Fail to determine X boundaries for group:" << (*gpit).first << "\n";
      mess << "The boundaries are (Xmin,Xmax):" << Xmin << " " << Xmax;
      throw std::runtime_error(mess.str());
    }
    step = (log(Xmax) - log(Xmin)) / (nPoints - 1);
    mess << "Found Group:" << ((*gpit).first) << "(Xmin,Xmax,log step):" << ((*gpit).second).first
        << "," << ((*gpit).second).second << "," << step;
    g_log.information(mess.str());
    mess.str("");
    boost::shared_ptr<MantidVec> xnew = boost::shared_ptr<MantidVec>(new MantidVec(nPoints)); //New X vector
    (*xnew)[0] = Xmin;
    for (int j = 1; j < nPoints; j++)
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

///Verify that all the contributing detectors to a spectrum belongs to the same group
/// @param spectrum_number The spectrum number in the workspace
/// @return Group number if successful otherwise return -1
int DiffractionFocussing2::validateSpectrumInGroup(int spectrum_number)
{
  // Get the spectra to detector map
  const API::SpectraDetectorMap& spectramap = inputW->spectraMap();
  const std::vector<int> dets = spectramap.getDetectors(spectrum_number);
  if (dets.empty()) // Not in group
    return -1;

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

} // namespace Algorithm
} // namespace Mantid
