//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include <set>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MaskDetectors)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace DataObjects;

/// (Empty) Constructor
MaskDetectors::MaskDetectors() {}

/// Destructor
MaskDetectors::~MaskDetectors() {}

void MaskDetectors::init()
{
  declareProperty(
    new WorkspaceProperty<>("Workspace","", Direction::InOut),
    "The name of the workspace that will be used as input and\n"
    "output for the algorithm" );
  declareProperty(new ArrayProperty<int>("SpectraList"),
    "A comma separated list or array containing a list of spectra to\n"
    "mask (DetectorList and WorkspaceIndexList are ignored if this\n"
    "is set)" );
  declareProperty(new ArrayProperty<int>("DetectorList"),
    "A comma separated list or array containing a list of detector ID's\n"
    "to mask (WorkspaceIndexList is ignored if this is set)" );
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "A comma separated list or array containing the workspace indices\n"
    "to mask" );
}

void MaskDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("Workspace");
  // Get the size of the vectors
  const int vectorSize = WS->blocksize();

  //Is it an event workspace?
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(WS);

  std::vector<int> indexList = getProperty("WorkspaceIndexList");
  std::vector<int> spectraList = getProperty("SpectraList");
  const std::vector<int> detectorList = getProperty("DetectorList");

  //each one of these values is optional but the user can't leave all three blank
  if ( indexList.empty() && spectraList.empty() && detectorList.empty() )
  {
    g_log.information(name() + ": There is nothing to mask, the index, spectra and detector lists are all empty");
    return;
  }

  // If the spectraList property has been set, need to loop over the workspace looking for the
  // appropriate spectra number and adding the indices they are linked to the list to be processed
  if ( ! spectraList.empty() )
  {
    fillIndexListFromSpectra(indexList,spectraList,WS);
  }// End dealing with spectraList
  else if ( ! detectorList.empty() )
  {// Dealing with DetectorList
    //convert from detectors to spectra numbers
    std::vector<int> mySpectraList = WS->spectraMap().getSpectra(detectorList);
    //then from spectra numbers to indices
    fillIndexListFromSpectra(indexList,mySpectraList,WS);
  }

  // Need to get hold of the parameter map
  Geometry::ParameterMap& pmap = WS->instrumentParameters();
  
  // If explicitly given a list of detectors to mask, just mark those.
  // Otherwise, mask all detectors pointing to the requested spectra in indexlist loop below
  bool detsMasked = false;
  std::vector<int>::const_iterator it;
  boost::shared_ptr<Instrument> instrument = WS->getBaseInstrument();
  if ( !detectorList.empty() )
  {
    for (it = detectorList.begin(); it != detectorList.end(); ++it)
    {
      try
      {
        if ( Geometry::Detector* det = dynamic_cast<Geometry::Detector*>(instrument->getDetector(*it).get()) )
        {
          pmap.addBool(det,"masked",true);
        }
      }
      catch(Kernel::Exception::NotFoundError &e)
      {
        g_log.warning() << e.what() << " Found while running MaskDetectors" << std::endl;
      }
    }
    detsMasked = true;
  }
  
  if ( indexList.size() == 0 )
  {
      g_log.warning("No spectra affected.");
      return;
  }
  
  // Get a reference to the spectra-detector map to get hold of detector ID's
  const SpectraDetectorMap& specMap = WS->spectraMap();
  double prog=0.0;
  for (it = indexList.begin(); it != indexList.end(); ++it)
  {
    if (!detsMasked)
    {
      // In this case, mask all detectors contributing to spectrum
      int spectrum_number = WS->getAxis(1)->spectraNo(*it);
      const std::vector<int> dets = specMap.getDetectors(spectrum_number);
      for (std::vector<int>::const_iterator iter=dets.begin(); iter != dets.end(); ++iter)
      {
        try
        {
          if ( Geometry::Detector* det = dynamic_cast<Geometry::Detector*>(instrument->getDetector(*iter).get()) )
          {
            pmap.addBool(det,"masked",true);
          }
        }
        catch(Kernel::Exception::NotFoundError &e)
        {
          g_log.warning() << e.what() << " Found while running MaskDetectors" << std::endl;
        }
      }
    }
    
    if (eventWS)
    {
      //Valid event workspace - clear the event list.
      eventWS->getEventList(*it).clear();
    }
    else
    {
      // Zero the workspace spectra (data and errors, not X values)
      WS->dataY(*it).assign(vectorSize,0.0);
      WS->dataE(*it).assign(vectorSize,0.0);
    }

    //Progress
    prog+=(double( 1)/indexList.size());
    progress(prog);
  }

  if (eventWS)
  {
    //Also clear the MRU for event workspaces.
    eventWS->clearMRU();
  }

}

/// Convert a list of spectra numbers into the corresponding workspace indices
void MaskDetectors::fillIndexListFromSpectra(std::vector<int>& indexList, std::vector<int>& spectraList,
                                              const API::MatrixWorkspace_sptr WS)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int> spectraSet(spectraList.begin(),spectraList.end());
  // Next line means that anything in Clear the index list first
  indexList.clear();
  indexList.reserve(WS->getNumberHistograms());
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
