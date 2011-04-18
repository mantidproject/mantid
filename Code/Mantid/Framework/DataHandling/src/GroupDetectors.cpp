//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include <set>
#include <numeric>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(GroupDetectors)

/// Sets documentation strings for this algorithm
void GroupDetectors::initDocs()
{
  this->setWikiSummary("Sums spectra bin-by-bin, equivalent to grouping the data from a set of detectors.  Individual groups can be specified by passing the algorithm a list of spectrum numbers, detector IDs or workspace indices. Many spectra groups can be created in one execution via an input file. ");
  this->setOptionalMessage("Sums spectra bin-by-bin, equivalent to grouping the data from a set of detectors.  Individual groups can be specified by passing the algorithm a list of spectrum numbers, detector IDs or workspace indices. Many spectra groups can be created in one execution via an input file.");
}


using namespace Kernel;
using namespace API;

/// (Empty) Constructor
GroupDetectors::GroupDetectors() {}

/// Destructor
GroupDetectors::~GroupDetectors() {}

void GroupDetectors::init()
{
  declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut,
    new CommonBinsValidator<>),
    "The name of the workspace2D on which to perform the algorithm");
  declareProperty(new ArrayProperty<int>("SpectraList"),
    "An array containing a list of the indexes of the spectra to combine\n"
    "(DetectorList and WorkspaceIndexList are ignored if this is set)" );
  declareProperty(new ArrayProperty<int>("DetectorList"), 
    "An array of detector ID's (WorkspaceIndexList is ignored if this is\n"
    "set)" );
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "An array of workspace indices to combine" );
  declareProperty("ResultIndex", -1,
    "The workspace index of the summed spectrum (or -1 on error)",
    Direction::Output);
}

void GroupDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("Workspace");

  std::vector<int> indexList = getProperty("WorkspaceIndexList");
  std::vector<int> spectraList = getProperty("SpectraList");
  const std::vector<int> detectorList = getProperty("DetectorList");

  // Could create a Validator to replace the below
  if ( indexList.empty() && spectraList.empty() && detectorList.empty() )
  {
    g_log.information(name() +
      ": WorkspaceIndexList, SpectraList, and DetectorList properties are all empty, no grouping done");
    return;
  }

  // Bin boundaries need to be the same, so check if they actually are
  if (!API::WorkspaceHelpers::commonBoundaries(WS))
  {
    g_log.error("Can only group if the histograms have common bin boundaries");
    throw std::runtime_error("Can only group if the histograms have common bin boundaries");
  }

  // Get hold of the axis that holds the spectrum numbers
  Axis *spectraAxis = WS->getAxis(1);

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! spectraList.empty() )
  {
    WS->getIndicesFromSpectra(spectraList,indexList);
  }// End dealing with spectraList
  else if ( ! detectorList.empty() )
  {// Dealing with DetectorList
    //convert from detectors to spectra numbers
    std::vector<int> mySpectraList = WS->spectraMap().getSpectra(detectorList);
    //then from spectra numbers to indices
    WS->getIndicesFromSpectra(mySpectraList,indexList);
  }

  if ( indexList.size() == 0 )
  {
      g_log.warning("Nothing to group");
      return;
  }

  const int vectorSize = WS->blocksize();
  const int firstIndex = indexList[0];
  const int firstSpectrum = spectraAxis->spectraNo(firstIndex);
  setProperty("ResultIndex",firstIndex);
  // loop over the spectra to group
  Progress progress(this, 0.0, 1.0, static_cast<int>(indexList.size()-1));
  for (unsigned int i = 0; i < indexList.size()-1; ++i)
  {
    const int currentIndex = indexList[i+1];
    // Move the current detector to belong to the first spectrum
    WS->mutableSpectraMap().remap(spectraAxis->spectraNo(currentIndex),firstSpectrum);
    // Add up all the Y spectra and store the result in the first one
    // Need to keep the next 3 lines inside loop for now until ManagedWorkspace mru-list works properly
    MantidVec &firstY = WS->dataY(firstIndex);
    MantidVec::iterator fYit;
    MantidVec::iterator fEit = WS->dataE(firstIndex).begin();
    MantidVec::iterator Yit = WS->dataY(currentIndex).begin();
    MantidVec::iterator Eit = WS->dataE(currentIndex).begin();
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

	progress.report();
  }

}

} // namespace DataHandling
} // namespace Mantid

