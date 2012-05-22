/*WIKI* 


This algorithm will flag the detectors listed as masked([[IDetector]] isMasked() method) and will zero the data in the spectra related to those detectors.

The set of detectors to be masked can be given as a list of either spectrum numbers, detector IDs or workspace indices. The list should be set against the appropriate property.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include <set>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MaskDetectors)

/// Sets documentation strings for this algorithm
void MaskDetectors::initDocs()
{
  this->setWikiSummary("An algorithm to mask a detector, or set of detectors, as not to be used. The workspace spectra associated with those detectors are zeroed. ");
  this->setOptionalMessage("An algorithm to mask a detector, or set of detectors, as not to be used. The workspace spectra associated with those detectors are zeroed.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument_const_sptr;
using Geometry::IDetector_const_sptr;
using namespace DataObjects;

/// (Empty) Constructor
MaskDetectors::MaskDetectors() {}

/// Destructor
MaskDetectors::~MaskDetectors() {}

/*
 * Define input arguments
 */
void MaskDetectors::init()
{
  declareProperty(
    new WorkspaceProperty<>("Workspace","", Direction::InOut),
    "The name of the workspace that will be used as input and\n"
    "output for the algorithm" );
  declareProperty(new ArrayProperty<specid_t>("SpectraList"),
    "A comma separated list or array containing a list of spectra to\n"
    "mask (DetectorList and WorkspaceIndexList are ignored if this\n"
    "is set)" );
  declareProperty(new ArrayProperty<detid_t>("DetectorList"),
    "A comma separated list or array containing a list of detector ID's\n"
    "to mask (WorkspaceIndexList is ignored if this is set)" );
  declareProperty(new ArrayProperty<size_t>("WorkspaceIndexList"),
    "A comma separated list or array containing the workspace indices\n"
    "to mask" );
  declareProperty(
    new WorkspaceProperty<>("MaskedWorkspace","",Direction::Input, PropertyMode::Optional),
    "If given, the masking from this workspace will be copied.");

  auto mustBePosInt = boost::make_shared<BoundedValidator<int> >();
  mustBePosInt->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
          "The index number of the first workspace index to include in the calculation\n"
          "(default 0)" );
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
          "The index number of the last workspace index to include in the calculation\n"
          "(default the last histogram)" );
}

/*
 * Main exec body
 */
void MaskDetectors::exec()
{
  // 1. Get the input workspace
  const MatrixWorkspace_sptr WS = getProperty("Workspace");
  //    Is it an event workspace?
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(WS);
  //    Is it a Mask Workspace ?
  MaskWorkspace_sptr isMaskWS = boost::dynamic_pointer_cast<MaskWorkspace>(WS);

  // 2. Get information for masking
  std::vector<size_t> indexList = getProperty("WorkspaceIndexList");
  std::vector<specid_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  const MatrixWorkspace_sptr inputMaskWS = getProperty("MaskedWorkspace");

  g_log.debug() << "MaskDetectors(): Input WorkspaceIndexList Size = " << indexList.size() <<
      "  SpectraList Size = " << spectraList.size() <<
      "  DetectorList Size = " << detectorList.size() << std::endl;

  // 3. Each one of these values is optional but the user can't leave all four blank
  if ( indexList.empty() && spectraList.empty() && detectorList.empty() && !inputMaskWS )
  {
    g_log.error(name() + ": There is nothing to mask, the index, spectra, "
		      "detector lists and masked workspace properties are all empty");
    return;
  }

  if (!spectraList.empty() && !indexList.empty())
  {
    g_log.error() << "It is not allowed to input both spectra list and workspace indices list. " << std::endl;
    throw std::invalid_argument("Not allowed to input both spectra list and workspace indices list.");
  }

  // 4. Create MaskWorkspace if necessary
  bool createmaskws = true;
  if (inputMaskWS)
  {
    mMaskWS = boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(inputMaskWS);
    if (mMaskWS)
    {
      if (mMaskWS->getNumberHistograms() != WS->getInstrument()->getDetectorIDs(true).size())
      {
        g_log.error() << "Input MaskWorkspace has different number of workspace indices "
            << mMaskWS->getNumberHistograms() << " than number of detectors of workspace to mask "
            << WS->getInstrument()->getDetectorIDs().size() << std::endl;
        throw std::invalid_argument("Unmatched instrument of mask workspace and workspace to mask.");
      }
      createmaskws = false;
    }
  }
  if (createmaskws)
  {
    mMaskWS = createMaskWorkspace(WS->getInstrument());
  }
  g_log.debug() << "MaskDetectors():  Internal mask workspace size = " << mMaskWS->getNumberHistograms() << std::endl;

  // 5. Apply spectra/workspace indices
  if ( ! spectraList.empty() )
  {
    // a) If the spectraList property has been set, need to loop over the workspace looking for the
    //    appropriate spectra number and adding the indices they are linked to the list to be processed
    fillIndexListFromSpectra(indexList,spectraList,WS);
  }
  // b) Sort the index list and remove duplicated
  std::vector<size_t> cleanedindices;
  if (indexList.size() > 0)
  {
    g_log.debug() << "MaskDetectros()  List of index Size = " << indexList.size() << " to sort. "<< std::endl;
    std::sort(indexList.begin(), indexList.end());
    std::vector<size_t>::iterator vit;
    size_t numdeleted = 0;
    g_log.debug() << "MaskDetectros()  List of index Size start to have duplicated removed. " << std::endl;

    cleanedindices.push_back(indexList[0]);

    for (size_t i = 1; i < indexList.size(); ++i)
    {
      if (indexList[i] != indexList[i-1])
      {
        cleanedindices.push_back(indexList[i]);
      }
      else
      {
        numdeleted ++;
      }
    }
    if (numdeleted > 0)
    {
      g_log.debug() << "MaskDetectors():  " << numdeleted << " input workspace indices are duplicated and thus deleted. " << std::endl;
    }
    else
    {
      g_log.debug() << "MaskDetectors(): No workspace index removed from list due to duplicate." << std::endl;
    }
  }

  // c) Put to the workspaces
  if (cleanedindices.size() > 0)
    applyIndexListToMaskWorkspace(WS, cleanedindices);

  // 6. Detectors List
  if ( !detectorList.empty() )
  {
    applyDetectorListToMaskWorkspace(detectorList);
  }

  // 7. Non-MaskWorkspace masking workspace
  if (inputMaskWS && createmaskws)
    applyWorkspaceToMaskWorkspace(inputMaskWS);

  // 8. Apply Mask to input
  maskWorkspace(WS);
  
  // 9. Finalize
  // a) Event workspace to clear MRU
  if (eventWS)
  {
    //Also clear the MRU for event workspaces.
    eventWS->clearMRU();
  }

  // b) If the input was a mask workspace, then extract the mask to ensure
  //     we are returning the correct thing.
  if (isMaskWS)
  {
    IAlgorithm_sptr alg = createSubAlgorithm("ExtractMask");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
    alg->executeAsSubAlg();
    MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
    setProperty("Workspace", ws);
  }

  // c) This rebuild request call, gives the workspace the opportunity to rebuild the nearest neighbours map
  //    and therfore pick up any detectors newly masked with this algorithm.
  WS->rebuildNearestNeighbours();

  return;
}

/**
 * Convert a list of spectra numbers into the corresponding workspace indices
 * @param indexList :: An output index list from the given spectra list
 * @param spectraList :: A list of spectra numbers
 * @param WS :: The input workspace to be masked
 */
void MaskDetectors::fillIndexListFromSpectra(std::vector<size_t>& indexList, const std::vector<specid_t>& spectraList,
					     const API::MatrixWorkspace_sptr WS)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> spectraSet(spectraList.begin(),spectraList.end());
  // Next line means that anything in Clear the index list first
  indexList.clear();
  indexList.reserve(WS->getNumberHistograms());

  for (int i = 0; i < static_cast<int>(WS->getNumberHistograms()); ++i)
  {
    const specid_t currentSpec = WS->getSpectrum(i)->getSpectrumNo();
    if ( spectraSet.find(currentSpec) != spectraSet.end() )
    {
      indexList.push_back(i);
    }
  }
}

/**
 * Append the indices of the masked spectra from the given workspace list to the given list
 * @param indexList :: An existing list of indices
 * @param maskedWorkspace :: An workspace with masked spectra
 */
void MaskDetectors::appendToIndexListFromWS(std::vector<size_t>& indexList, const MatrixWorkspace_sptr maskedWorkspace)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> existingIndices(indexList.begin(), indexList.end());
  int endIndex = getProperty("EndWorkspaceIndex");
  if (endIndex == EMPTY_INT() ) endIndex = static_cast<int>(maskedWorkspace->getNumberHistograms() - 1);
  int startIndex = getProperty("StartWorkspaceIndex");

  
  for (int64_t i = startIndex; i <= endIndex; ++i)
  {
    IDetector_const_sptr det;
    try
    {
      det = maskedWorkspace->getDetector(i-startIndex);
    }
    catch(Exception::NotFoundError &)
    {
      continue;
    }

    if( det->isMasked() && existingIndices.count(i) == 0 )
    {
      indexList.push_back(i);
    }
  }

  return;
} // appendToIndexListFromWS

/**
 * Append the indices of the masked spectra from the given workspace list to the given list
 * @param indexList :: An existing list of indices
 * @param maskedWorkspace :: An workspace with masked spectra
 */
void MaskDetectors::appendToIndexListFromMaskWS(std::vector<size_t>& indexList, const DataObjects::MaskWorkspace_const_sptr maskedWorkspace)
{
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> existingIndices(indexList.begin(), indexList.end());
  int endIndex = getProperty("EndWorkspaceIndex");
  if (endIndex == EMPTY_INT() ) endIndex = static_cast<int>(maskedWorkspace->getNumberHistograms() - 1);
  int startIndex = getProperty("StartWorkspaceIndex");

  for (int64_t i = startIndex; i <= endIndex; ++i)
  {

    if( maskedWorkspace->dataY(i-startIndex)[0] > 0.5 && existingIndices.count(i) == 0 )
    {
      g_log.debug() << "Adding WorkspaceIndex " << i << " to mask." << std::endl;
      indexList.push_back(i);
    }
  }

  return;
} // appendToIndexListFromWS


/*
 * Create an empty MaskWorkspace w/o any detectors masked.
 */
DataObjects::MaskWorkspace_sptr MaskDetectors::createMaskWorkspace(Geometry::Instrument_const_sptr instrument)
{
  //   Geometry::Instrument_const_sptr minstrument = tempWS->getInstrument();
  DataObjects::MaskWorkspace_sptr maskws =
      DataObjects::MaskWorkspace_sptr(new DataObjects::MaskWorkspace(instrument));
  for (size_t i = 0; i < maskws->getNumberHistograms(); ++i)
  {
    maskws->dataY(i)[0] = 0.0;
  }
  maskws->setTitle("Mask");

  return maskws;
}

/*
 * Apply the list of workspace indices to mask workspace
 */
void MaskDetectors::applyIndexListToMaskWorkspace(API::MatrixWorkspace_sptr inpWS, std::vector<size_t> indexList)
{
  // 1. Get map
  g_log.notice() << "MaskDetectors.applyIndexListToMaskWorkspace ... " << std::endl;
  detid2index_map *map = mMaskWS->getDetectorIDToWorkspaceIndexMap(false);
  if (!map)
  {
    g_log.error() << "MaskDetectors() Unable to get DetectorIDToWorkspaceIndex map()! " << std::endl;
    throw std::runtime_error("Unable to getDetectorIDtoWorkspaceIndexMap.");
  }

  // 2. Apply
  double prog=0.0;
  std::vector<size_t>::const_iterator wit;

  size_t numdettomask = 0;
  std::set<size_t> wsset;
  g_log.debug() << "MaskDetectors(): Number of WorkspaceIndex to Mask = " << indexList.size() << std::endl;
  for (size_t iws = 0; iws < indexList.size(); ++ iws)
  {
    // a) Get detectors
    API::ISpectrum* spec = inpWS->getSpectrum(indexList[iws]);
    std::set<detid_t> detids = spec->getDetectorIDs();

    // b) Apply to mask workspace
    std::set<detid_t>::iterator setit;
    for (setit = detids.begin(); setit != detids.end(); ++ setit)
    {
      detid_t detid = *setit;
      detid2index_map::iterator dit;
      dit = map->find(detid);
      if (dit != map->end())
      {
        size_t wsindex = dit->second;
        mMaskWS->dataY(wsindex)[0] = 1.0;
        wsset.insert(wsindex);
        ++ numdettomask;
      }
      else
      {
        g_log.warning() << "Detector w/ ID " << detid << " is not defined in instrument. " << std::endl;
      }
    }

    // c) Progress
    prog+= (0.25/static_cast<int>(indexList.size()));
    progress(prog);
  }

  g_log.debug() << "MaskDetectors(): " << numdettomask << " detectors are to be masked." <<
      " and there are " << wsset.size() << "  individual workspace indices" << std::endl;

  // 3. Clean
  delete map;

  return;
}

/*
 * Apply a list of detectors (ID) to mask workspace
 */
void MaskDetectors::applyDetectorListToMaskWorkspace(std::vector<detid_t> detectorList)
{
  // 1. Get map
  // detid2index_map *map = inpWS->getDetectorIDToWorkspaceIndexMap(false);
  detid2index_map *map = mMaskWS->getDetectorIDToWorkspaceIndexMap(false);

  // 2. Apply
  for (size_t idet = 0; idet < detectorList.size(); ++ idet)
  {
    detid_t detid = detectorList[idet];
    detid2index_map::iterator dit;
    dit = map->find(detid);
    if (dit != map->end())
    {
      size_t wsindex = dit->second;
      mMaskWS->dataY(wsindex)[0] = 1.0;
    }
    else
    {
      g_log.warning() << "Detector w/ ID " << detid << " is not defined in instrument. " << std::endl;
    }
  }

  // 3. Clean
  delete map;

  return;
}

/*
 * Apply a non-MaskWorkspace's masked workspace to WS
 */
void MaskDetectors::applyWorkspaceToMaskWorkspace(API::MatrixWorkspace_sptr maskedWorkspace)
{
  // Convert the vector of properties into a set for easy searching
  int endIndex = getProperty("EndWorkspaceIndex");
  if (endIndex == EMPTY_INT() )
    endIndex = static_cast<int>(maskedWorkspace->getNumberHistograms() - 1);
  int startIndex = getProperty("StartWorkspaceIndex");

  // 1. Get map
  detid2index_map *maskmap = mMaskWS->getDetectorIDToWorkspaceIndexMap(false);

  for (specid_t iws = startIndex; iws <= endIndex; ++iws)
  {
    // a) Get one detector to check whether the detector of this workspace is masked
    IDetector_const_sptr det;
    try
    {
      det = maskedWorkspace->getDetector(iws);
    }
    catch(Exception::NotFoundError &)
    {
      continue;
    }

    // b) If the 'one' detector is masked, then mask mask workspace
    if( det->isMasked())
    {
      API::ISpectrum* spec = maskedWorkspace->getSpectrum(iws);
      std::set<detid_t> detids = spec->getDetectorIDs();

      // b) Apply to mask workspace
      std::set<detid_t>::iterator setit;
      for (setit = detids.begin(); setit != detids.end(); ++ setit)
      {
        detid_t detid = *setit;
        detid2index_map::iterator dit;
        dit = maskmap->find(detid);
        if (dit != maskmap->end())
        {
          size_t wsindex = dit->second;
          mMaskWS->dataY(wsindex)[0] = 1.0;
        }
        else
        {
          g_log.warning() << "Detector w/ ID " << detid << " is not defined in instrument. " << std::endl;
        }
      } // FOR DetIDs
    } // ENDIF-masked
  } // ENDFOR workspace index

  // 3) Clean
  delete maskmap;

  return;
}

/*
 * Mask workspace via myMaskWorkspace
 */
void MaskDetectors::maskWorkspace(API::MatrixWorkspace_sptr inpWS)
{
  index2detid_map *maskmap = mMaskWS->getWorkspaceIndexToDetectorIDMap();
  index2detid_map::iterator mit;

  detid2index_map *inwsmap = inpWS->getDetectorIDToWorkspaceIndexMap(false);
  detid2index_map::iterator pit; 

  Geometry::Instrument_const_sptr instrument = inpWS->getInstrument();
  Geometry::ParameterMap& pmap = inpWS->instrumentParameters();

  // FIXME Efficiency + Correctness if grouped detectors
  double prog = 0.25;
  size_t numdetmasked = 0;
  size_t numdettomask = 0;
  for (size_t iws = 0; iws < mMaskWS->getNumberHistograms(); ++ iws)
  {
    if (mMaskWS->dataY(iws)[0] > 0.5)
    {
      numdettomask += 1;

      // If it is masked
      mit = maskmap->find(iws);
      if (mit == maskmap->end())
      {
        g_log.warning() << "Workspace Index " << iws << " has not detector ID associated!" << std::endl;
        continue;
      }
      detid_t detid = mit->second;
      try
      {
        const Geometry::ComponentID det = instrument->getDetector(detid)->getComponentID();
        if (det)
        {
          pmap.addBool(det, "masked", true);
        
          pit = inwsmap->find(detid);
          if (pit == inwsmap->end())
          {
              g_log.warning() << "InputWorkspace has no workspace associated with detector w/ ID " << detid << std::endl;
              continue;
          }
          specid_t newiws = static_cast<specid_t>(pit->second);
          ISpectrum *spec = inpWS->getSpectrum(newiws);
          if (!spec)
              throw std::invalid_argument("Got a null spectrum");
          else
              spec->clearData();

          ++ numdetmasked;
        }
        else
        {
          g_log.warning() << "Detector ID " << detid << " is not a component ID for detector. " << std::endl;
        }
      }
      catch(Kernel::Exception::NotFoundError &e)
      {
        g_log.warning() << e.what() << " Found while running MaskDetectors" << std::endl;
      }
    } // END IF

    // Report progress
    int denom = static_cast<int>(mMaskWS->getNumberHistograms()/10);
    if (denom == 0)
    {
      denom = 1;
    }
    if (iws % (denom) == 0)
    {
      prog+= (0.75/static_cast<double>(mMaskWS->getNumberHistograms()));
      progress(prog);
    }

  } // ENDFOR

  g_log.debug() << "Total " << numdetmasked << " detectors are masked. vs. " << numdettomask << " to mask." << std::endl;

  progress(1.0);

  // -1 Clean
  delete maskmap;
  delete inwsmap;

  return;
}


} // namespace DataHandling
} // namespace Mantid
