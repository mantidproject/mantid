//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include <set>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

// Initialise the logger
Kernel::Logger& GroupDetectors::g_log = Kernel::Logger::get("GroupDetectors");

/// (Empty) Constructor
GroupDetectors::GroupDetectors() {}

/// Destructor
GroupDetectors::~GroupDetectors() {}

void GroupDetectors::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","",Direction::InOut,
                  new CommonBinsValidator<Workspace2D_sptr>));
  declareProperty(new ArrayProperty<int>("SpectraList"));
  declareProperty(new ArrayProperty<int>("DetectorList"));
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"));
  declareProperty("ResultIndex",-1, Direction::Output);
}

void GroupDetectors::exec()
{
  // Get the input workspace
  const Workspace2D_sptr WS = getProperty("Workspace");

  Property *wil = getProperty("WorkspaceIndexList");
  Property *sl = getProperty("SpectraList");
  Property *dl = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( wil->isDefault() && sl->isDefault() && dl->isDefault() )
  {
    g_log.error("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
    throw std::invalid_argument("WorkspaceIndexList, SpectraList, and DetectorList properties are empty");
  }

  // Bin boundaries need to be the same, so check if they actually are
  if ( !hasSameBoundaries(WS) )
  {
    g_log.error("Can only group if the histograms have common bin boundaries");
    throw std::runtime_error("Can only group if the histograms have common bin boundaries");
  }

  std::vector<int> indexList = getProperty("WorkspaceIndexList");
  // Get hold of the axis that holds the spectrum numbers
  Axis *spectraAxis = WS->getAxis(1);

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! sl->isDefault() )
  {
    std::vector<int> spectraList = getProperty("SpectraList");
    fillIndexListFromSpectra(indexList,spectraList,WS);
  }// End dealing with spectraList
  else if ( ! dl->isDefault() )
  {// Dealing with DetectorList
    const std::vector<int> detectorList = getProperty("DetectorList");
    //convert from detectors to spectra numbers
    boost::shared_ptr<API::SpectraDetectorMap> spectraMap = WS->getSpectraMap();
    std::vector<int> mySpectraList = spectraMap->getSpectra(detectorList);
    //then from spectra numbers to indices
    fillIndexListFromSpectra(indexList,mySpectraList,WS);
  }

  if ( indexList.size() == 0 )
  {
      g_log.warning("Nothing to group");
      return;
  }

  // Copy the spectra-detector map because it's going to be changed
  WS->copySpectraMap(WS->getSpectraMap());

  const int vectorSize = WS->blocksize();
  const int firstIndex = indexList[0];
  const int firstSpectrum = spectraAxis->spectraNo(firstIndex);
  setProperty("ResultIndex",firstIndex);
  // loop over the spectra to group
  for (unsigned int i = 0; i < indexList.size()-1; ++i)
  {
    const int currentIndex = indexList[i+1];
    // Move the current detector to belong to the first spectrum
    WS->getSpectraMap()->remap(spectraAxis->spectraNo(currentIndex),firstSpectrum);
    // Add up all the Y spectra and store the result in the first one
    // Need to keep the next 3 lines inside loop for now until ManagedWorkspace mru-list works properly
    std::vector<double> &firstY = WS->dataY(firstIndex);
    std::vector<double>::iterator fYit;
    std::vector<double>::iterator fEit = WS->dataE(firstIndex).begin();
    std::vector<double>::iterator Yit = WS->dataY(currentIndex).begin();
    std::vector<double>::iterator Eit = WS->dataE(currentIndex).begin();
    for (fYit = firstY.begin(); fYit != firstY.end(); ++fYit, ++fEit, ++Yit, ++Eit)
    {
      *fYit += *Yit;
      // Assume 'normal' (i.e. Gaussian) combination of errors
      *fEit = sqrt( (*fEit)*(*fEit) + (*Eit)*(*Eit) );
    }
    // Now zero the now redundant spectrum and set its spectraNo to indicate this (using -1)
    // N.B. Deleting spectra would cause issues for ManagedWorkspace2D, hence the the approach taken here
    WS->dataY(currentIndex).assign(vectorSize,0.0);
    WS->dataE(currentIndex).assign(vectorSize,0.0);
    spectraAxis->spectraNo(currentIndex) = -1;
  }

}

/// Checks if all histograms have the same boundaries by comparing their sums
bool GroupDetectors::hasSameBoundaries(const DataObjects::Workspace2D_sptr WS)
{
  if (!WS->blocksize()) return true;
  const double commonSum = std::accumulate(WS->dataX(0).begin(),WS->dataX(0).end(),0.);
  const int numHist = WS->getNumberHistograms();
  for (int i = 1; i < numHist; ++i)
  {
    const double sum = std::accumulate(WS->dataX(i).begin(),WS->dataX(i).end(),0.);
    if ( std::abs(commonSum-sum) > 1.0E-9 ) return false;
  }
  return true;
}

/// Convert a list of spectra numbers into the corresponding workspace indices
void GroupDetectors::fillIndexListFromSpectra(std::vector<int>& indexList, std::vector<int>& spectraList,
                                              const DataObjects::Workspace2D_sptr WS)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int> spectraSet(spectraList.begin(),spectraList.end());
  // Next line means that anything in Clear the index list first
  indexList.clear();
  //get the spectra axis
  Axis *spectraAxis = WS->getAxis(1);

  for (int i = 0; i < WS->getNumberHistograms(); ++i)
  {
    int currentSpec = spectraAxis->spectraNo(i);
    if ( spectraSet.find(currentSpec) != spectraSet.end() )
    {
      indexList.push_back(i);
    }
  }
}


} // namespace DataHandling
} // namespace Mantid

