//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include <set>
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
GroupDetectors2::GroupDetectors2() {}

/// Destructor
GroupDetectors2::~GroupDetectors2() {}

void GroupDetectors2::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,
    new CommonBinsValidator<Workspace2D>),"The name of the input 2D workspace");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),"The name of the output workspace");
  declareProperty(new ArrayProperty<int>("SpectraList"),
    "An array containing a list of the indexes of the spectra to combine\n"
    "(DetectorList and WorkspaceIndexList are ignored if this is set)" );
  declareProperty(new ArrayProperty<int>("DetectorList"), 
    "An array of detector ID's (WorkspaceIndexList is ignored if this is\n"
    "set)" );
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "An array of workspace indices to combine" );
  declareProperty("KeepUngroupedSpectra",false,"If true, ungrouped spectra will be kept in the output workspace");
  declareProperty("ResultIndex", -1,
    "The workspace index of the summed spectrum (or -1 on error)",
    Direction::Output);
}

void GroupDetectors2::exec()
{
  // Get the input workspace
  const Workspace2D_const_sptr inputWS = getProperty("InputWorkspace");

  std::vector<int> indexList = getProperty("WorkspaceIndexList");
  std::vector<int> spectraList = getProperty("SpectraList");
  const std::vector<int> detectorList = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( indexList.empty() && spectraList.empty() && detectorList.empty() )
  {
    g_log.information(name() + ": WorkspaceIndexList, SpectraList, and DetectorList properties are all empty");
    throw std::runtime_error("WorkspaceIndexList, SpectraList, and DetectorList properties are all empty");
  }

  // Bin boundaries need to be the same, so do the full check on whether they actually are
  if (!API::WorkspaceHelpers::commonBoundaries(inputWS))
  {
    g_log.error("Can only group if the histograms have common bin boundaries");
    throw std::runtime_error("Can only group if the histograms have common bin boundaries");
  }

  // Get hold of the axis that holds the spectrum numbers
  Axis *spectraAxis = inputWS->getAxis(1);

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! spectraList.empty() )
  {
    WorkspaceHelpers::getIndicesFromSpectra(inputWS,spectraList,indexList);
  }// End dealing with spectraList
  else if ( ! detectorList.empty() )
  {// Dealing with DetectorList
    //convert from detectors to spectra numbers
    std::vector<int> mySpectraList = inputWS->spectraMap().getSpectra(detectorList);
    //then from spectra numbers to indices
    WorkspaceHelpers::getIndicesFromSpectra(inputWS,mySpectraList,indexList);
  }

  if ( indexList.empty() )
  {
      g_log.error("Nothing to group");
      throw std::runtime_error("Nothing to group");
  }
  
  // Convert the indexList vector into a set for better searching
  const std::set<int> indices(indexList.begin(), indexList.end());
  if (indices.size() != indexList.size()) g_log.warning("Duplicate indices in input index list");
  
  // Check what we're doing with ungrouped spectra
  const bool keepAll = getProperty("KeepUngroupedSpectra");
  
  // Create the output workspace.
  const int numHists = inputWS->getNumberHistograms();
  const int newSize = keepAll ? (numHists - indices.size() + 1) : 1;
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,newSize,inputWS->readX(0).size(),inputWS->blocksize());

  // Get a reference to the spectra map on the output workspace
  SpectraDetectorMap &specMap = outputWS->mutableSpectraMap();
  
  const int firstIndex = keepAll ? *(indices.begin()) : 0;
  // Copy over X data in first spectrum
  outputWS->dataX(firstIndex) = inputWS->readX(*(indices.begin()));
  const int firstSpectrum = spectraAxis->spectraNo(firstIndex);
  setProperty("ResultIndex",firstIndex);
  // loop over the spectra to group
  Progress progress(this,0.0,1.0,numHists);
  for (unsigned int i = 0; i < indexList.size(); ++i)
  {
    const int currentIndex = indexList[i];
    // Move the current detector to belong to the first spectrum
    specMap.remap(spectraAxis->spectraNo(currentIndex),firstSpectrum);
    // Add up all the Y spectra and store the result in the first one
    // Need to keep the next 3 lines inside loop for now until ManagedWorkspace mru-list works properly
    MantidVec &firstY = outputWS->dataY(firstIndex);
    MantidVec::iterator fYit;
    MantidVec::iterator fEit = outputWS->dataE(firstIndex).begin();
    MantidVec::const_iterator Yit = inputWS->readY(currentIndex).begin();
    MantidVec::const_iterator Eit = inputWS->readE(currentIndex).begin();
    for (fYit = firstY.begin(); fYit != firstY.end(); ++fYit, ++fEit, ++Yit, ++Eit)
    {
      *fYit += *Yit;
      // Assume 'normal' (i.e. Gaussian) combination of errors
      *fEit = std::sqrt( (*fEit)*(*fEit) + (*Eit)*(*Eit) );
    }

    progress.report();
  }
  
  // If we're keeping ungrouped spectra, copy them all over
  if (keepAll)
  {
    int currentOutIndex = 0;
    for (int i = 0; i < numHists; ++i)
    {
      if ( i == firstIndex ) ++currentOutIndex;
      else if ( indices.count(i) == 0 )
      {
        outputWS->dataX(currentOutIndex) = inputWS->readX(i);
        outputWS->dataY(currentOutIndex) = inputWS->readY(i);
        outputWS->dataE(currentOutIndex) = inputWS->readE(i);
        outputWS->getAxis(1)->spectraNo(currentOutIndex) = spectraAxis->spectraNo(i);
        ++currentOutIndex;
      }
      progress.report();
    }    
  }

  setProperty("OutputWorkspace",outputWS);
}

} // namespace DataHandling
} // namespace Mantid

