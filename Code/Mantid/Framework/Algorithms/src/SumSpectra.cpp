/*WIKI* 

Takes a workspace as input and sums all of the spectra within it maintaining the existing bin structure and units. Any masked spectra are ignored.
The result is stored as a new workspace containing a single spectra.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataObjects/RebinnedOutput.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

/// Sets documentation strings for this algorithm
void SumSpectra::initDocs()
{
  this->setWikiSummary("The SumSpectra algorithm adds the data values in each time bin across a range of spectra; the output workspace has a single spectrum. If the input is an [[EventWorkspace]], the output is also an [[EventWorkspace]]; otherwise it will be a [[Workspace2D]]. ");
  this->setOptionalMessage("The SumSpectra algorithm adds the data values in each time bin across a range of spectra; the output workspace has a single spectrum. If the input is an EventWorkspace, the output is also an EventWorkspace; otherwise it will be a Workspace2D.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void SumSpectra::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input, boost::make_shared<CommonBinsValidator>()),
                            "The workspace containing the spectra to be summed" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,
    "The first Workspace index to be included in the summing (default 0)" );
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive,
    "The last Workspace index to be included in the summing (default\n"
    "highest index)" );

  declareProperty(new Kernel::ArrayProperty<int>("ListOfWorkspaceIndices"),
    "A list of workspace indices as a string with ranges; e.g. 5-10,15,20-23. \n"
    "Can be specified instead of in addition to StartWorkspaceIndex and EndWorkspaceIndex.");

  declareProperty("IncludeMonitors",true,"Whether to include monitor spectra in the sum (default: yes)");
}

/** Executes the algorithm
 *
 */
void SumSpectra::exec()
{
  // Try and retrieve the optional properties
  m_MinSpec = getProperty("StartWorkspaceIndex");
  m_MaxSpec = getProperty("EndWorkspaceIndex");
  const std::vector<int> indices_list = getProperty("ListOfWorkspaceIndices");

  keepMonitors = getProperty("IncludeMonitors");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  numberOfSpectra = static_cast<int>(localworkspace->getNumberHistograms());
  this->yLength = static_cast<int>(localworkspace->blocksize());

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_MinSpec > numberOfSpectra )
  {
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
    m_MinSpec = 0;
  }

  if (indices_list.empty())
  {
    //If no list was given and no max, just do all.
    if ( isEmpty(m_MaxSpec) ) m_MaxSpec = numberOfSpectra-1;
  }

  //Something for m_MaxSpec was given but it is out of range?
  if (!isEmpty(m_MaxSpec) && ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec ))
  {
    g_log.warning("EndWorkspaceIndex out of range! Set to max Workspace Index");
    m_MaxSpec = numberOfSpectra;
  }

  //Make the set of indices to sum up from the list
  this->indices.insert(indices_list.begin(), indices_list.end());

  //And add the range too, if any
  if (!isEmpty(m_MaxSpec))
  {
    for (int i = m_MinSpec; i <= m_MaxSpec; i++)
      this->indices.insert(i);
  }
  
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(localworkspace);
  if (eventW)
  {
    this->execEvent(eventW, this->indices);
  }
  else
  {
    //-------Workspace 2D mode -----

    // Create the 2D workspace for the output
    MatrixWorkspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,
                                                           1,localworkspace->readX(0).size(),this->yLength);

    Progress progress(this, 0, 1, this->indices.size());

    // This is the (only) output spectrum
    ISpectrum * outSpec = outputWorkspace->getSpectrum(0);

    // Copy over the bin boundaries
    outSpec->dataX() = localworkspace->readX(0);

    //Build a new spectra map
    specid_t newSpectrumNo = m_MinSpec;
    outSpec->setSpectrumNo(newSpectrumNo);
    outSpec->clearDetectorIDs();
    g_log.information() << "Spectra remapping gives single spectra with spectra number: " << newSpectrumNo << "\n";

    if (localworkspace->id() == "RebinnedOutput")
    {
      this->doRebinnedOutput(outputWorkspace, progress);
    }
    else
    {
      this->doWorkspace2D(localworkspace, outSpec, progress);
    }

    // Pointer to sqrt function
    MantidVec& YError = outSpec->dataE();
    typedef double (*uf)(double);
    uf rs=std::sqrt;
    //take the square root of all the accumulated squared errors - Assumes Gaussian errors
    std::transform(YError.begin(), YError.end(), YError.begin(), rs);

    outputWorkspace->generateSpectraMap();

    // Assign it to the output workspace property
    setProperty("OutputWorkspace", outputWorkspace);

  }
}

/**
 * This function deals with the logic necessary for summing a Workspace2D.
 * @param localworkspace the input workspace for summing
 * @param outSpec the spectrum for the summed output
 * @param progress the progress indicator
 */
void SumSpectra::doWorkspace2D(MatrixWorkspace_const_sptr localworkspace,
                               ISpectrum *outSpec, Progress &progress)
{
  // Get references to the output workspaces's data vectors
  MantidVec& YSum = outSpec->dataY();
  MantidVec& YError = outSpec->dataE();

  // Loop over spectra
  std::set<int>::iterator it;
  //for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  for (it = this->indices.begin(); it != this->indices.end(); ++it)
  {
    int i =  *it;
    //Don't go outside the range.
    if ((i >= this->numberOfSpectra) || (i < 0))
    {
      g_log.error() << "Invalid index " << i << " was specified. Sum was aborted.\n";
      break;
    }

    try
    {
      // Get the detector object for this spectrum
      Geometry::IDetector_const_sptr det = localworkspace->getDetector(i);
      // Skip monitors, if the property is set to do so
      if ( !keepMonitors && det->isMonitor() ) continue;
      // Skip masked detectors
      if ( det->isMasked() ) continue;
    }
    catch(...)
    {
      // if the detector not found just carry on
    }

    // Retrieve the spectrum into a vector
    const MantidVec& YValues = localworkspace->readY(i);
    const MantidVec& YErrors = localworkspace->readE(i);

    for (int k = 0; k < this->yLength; ++k)
    {
      YSum[k] += YValues[k];
      YError[k] += YErrors[k]*YErrors[k];
    }

    // Map all the detectors onto the spectrum of the output
    outSpec->addDetectorIDs( localworkspace->getSpectrum(i)->getDetectorIDs() );

    progress.report();
  }
}

/**
 * This function handles the logic for summing RebinnedOutput workspaces.
 * @param outputWorkspace the workspace to hold the summed input
 * @param progress the progress indicator
 */
void SumSpectra::doRebinnedOutput(MatrixWorkspace_sptr outputWorkspace,
                                  Progress &progress)
{
  // Get a mutable copy of the input workspace
  MatrixWorkspace_sptr localworkspace = getProperty("InputWorkspace");

  // First, we need to clean the input workspace for nan's and inf's in order
  // to treat the data correctly later.
  IAlgorithm_sptr alg = this->createSubAlgorithm("ReplaceSpecialValues");
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", localworkspace);
  alg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", localworkspace);
  alg->setProperty("NaNValue", 0.0);
  alg->setProperty("NaNError", 0.0);
  alg->setProperty("InfinityValue", 0.0);
  alg->setProperty("InfinityError", 0.0);
  alg->executeAsSubAlg();

  // Transform to real workspace types
  RebinnedOutput_sptr inWS = boost::dynamic_pointer_cast<RebinnedOutput>(localworkspace);
  RebinnedOutput_sptr outWS = boost::dynamic_pointer_cast<RebinnedOutput>(outputWorkspace);

  // Get references to the output workspaces's data vectors
  ISpectrum* outSpec = outputWorkspace->getSpectrum(0);
  MantidVec& YSum = outSpec->dataY();
  MantidVec& YError = outSpec->dataE();
  MantidVec& FracSum = outWS->dataF(0);

  // Loop over spectra
  std::set<int>::iterator it;
  //for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  for (it = indices.begin(); it != indices.end(); ++it)
  {
    int i =  *it;
    //Don't go outside the range.
    if ((i >= numberOfSpectra) || (i < 0))
    {
      g_log.error() << "Invalid index " << i << " was specified. Sum was aborted.\n";
      break;
    }

    try
    {
      // Get the detector object for this spectrum
      Geometry::IDetector_const_sptr det = localworkspace->getDetector(i);
      // Skip monitors, if the property is set to do so
      if ( !keepMonitors && det->isMonitor() ) continue;
      // Skip masked detectors
      if ( det->isMasked() ) continue;
    }
    catch(...)
    {
      // if the detector not found just carry on
    }

    // Retrieve the spectrum into a vector
    const MantidVec& YValues = localworkspace->readY(i);
    const MantidVec& YErrors = localworkspace->readE(i);
    const MantidVec& FracArea = inWS->readF(i);

    for (int k = 0; k < this->yLength; ++k)
    {
      YSum[k] += YValues[k] * FracArea[k];
      YError[k] += YErrors[k] * YErrors[k] * FracArea[k] * FracArea[k];
      FracSum[k] += FracArea[k];
    }

    // Map all the detectors onto the spectrum of the output
    outSpec->addDetectorIDs(localworkspace->getSpectrum(i)->getDetectorIDs());

    progress.report();
  }
  // Create the correct representation
  outWS->finalize();
}

/** Executes the algorithm
 *@param localworkspace :: the input workspace
 *@param indices :: set of indices to sum up
 */
void SumSpectra::execEvent(EventWorkspace_const_sptr localworkspace, std::set<int> &indices)
{
  //Make a brand new EventWorkspace
  EventWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace", 1, 2, 1));
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(localworkspace, outputWorkspace, true);

  Progress progress(this,0,1, indices.size());

  //Get the pointer to the output event list
  EventList & outEL = outputWorkspace->getEventList(0);
  outEL.setSpectrumNo(m_MinSpec);
  outEL.clearDetectorIDs();

  // Loop over spectra
  std::set<int>::iterator it;
  //for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  for (it = indices.begin(); it != indices.end(); ++it)
  {
    int i =  *it;
    //Don't go outside the range.
    if ((i >= numberOfSpectra) || (i < 0))
    {
      g_log.error() << "Invalid index " << i << " was specified. Sum was aborted.\n";
      break;
    }

    try
    {
      // Get the detector object for this spectrum
      Geometry::IDetector_const_sptr det = localworkspace->getDetector(i);
      // Skip monitors, if the property is set to do so
      if ( !keepMonitors && det->isMonitor() ) continue;
      // Skip masked detectors
      if ( det->isMasked() ) continue;
    }
    catch(...)
    {
      // if the detector not found just carry on
    }
    //Add the event lists with the operator
    outEL += localworkspace->getEventList(i);

    progress.report();
  }

  //Finalize spectra map etc.
  outputWorkspace->doneAddingEventLists();

  //Set all X bins on the output
  cow_ptr<MantidVec> XValues;
  XValues.access() = localworkspace->readX(0);
  outputWorkspace->setAllX(XValues);

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",boost::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace));

}


} // namespace Algorithms
} // namespace Mantid
