//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void SumSpectra::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new CommonBinsValidator<>),
                            "The workspace containing the spectra to be summed" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,
    "The first Workspace index to be included in the summing (default 0)" );
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive->clone(),
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

  numberOfSpectra = localworkspace->getNumberHistograms();
  const int YLength = localworkspace->blocksize();

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_MinSpec > numberOfSpectra )
  {
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
    m_MinSpec = 0;
  }

  if (indices_list.size() == 0)
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
  std::set<int> indices(indices_list.begin(), indices_list.end());

  //And add the range too, if any
  if (!isEmpty(m_MaxSpec))
  {
    for (int i = m_MinSpec; i <= m_MaxSpec; i++)
      indices.insert(i);
  }
  
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(localworkspace);
  if (eventW)
  {
    this->execEvent(eventW, indices);
  }
  else
  {
    //-------Workspace 2D mode -----

    // Create the 2D workspace for the output
    MatrixWorkspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,
                                                           1,localworkspace->readX(0).size(),YLength);

    Progress progress(this,0,1, indices.size(), 1);

    // Copy over the bin boundaries
    outputWorkspace->dataX(0) = localworkspace->readX(0);
    // Get references to the output workspaces's data vectors
    MantidVec& YSum = outputWorkspace->dataY(0);
    MantidVec& YError = outputWorkspace->dataE(0);
    // Get a reference to the spectra-detector map
    SpectraDetectorMap& specMap = outputWorkspace->mutableSpectraMap();
    // Clear and add is orders of magnitude faster than remap()
    specMap.clear();
    const SpectraDetectorMap & inputSpecMap = localworkspace->spectraMap();
    const Axis* const spectraAxis = localworkspace->getAxis(1);
    int newSpectrumNo = 0;
    if ( spectraAxis->isSpectra() )
    {
      newSpectrumNo = spectraAxis->spectraNo(m_MinSpec);
      outputWorkspace->getAxis(1)->spectraNo(0) = newSpectrumNo;
    }
    g_log.information() << "Spectra remapping gives single spectra with spectra number: " << newSpectrumNo << "\n";

    // Loop over spectra
    std::set<int>::iterator it;
    //for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
    for (it = indices.begin(); it != indices.end(); it++)
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

      for (int k = 0; k < YLength; ++k)
      {
        YSum[k] += YValues[k];
        YError[k] += YErrors[k]*YErrors[k];
      }

      // Map all the detectors onto the spectrum of the output
      if (spectraAxis->isSpectra())
      {
        specMap.addSpectrumEntries(newSpectrumNo,inputSpecMap.getDetectors(spectraAxis->spectraNo(i)));
      }

      progress.report();
    }

    // Pointer to sqrt function
    typedef double (*uf)(double);
    uf rs=std::sqrt;
    //take the square root of all the accumulated squared errors - Assumes Gaussian errors
    std::transform(YError.begin(), YError.end(), YError.begin(), rs);

    // Assign it to the output workspace property
    setProperty("OutputWorkspace",outputWorkspace);

  }
}


/** Executes the algorithm
 *
 *@param indices set of indices to sum up
 */
void SumSpectra::execEvent(EventWorkspace_const_sptr localworkspace, std::set<int> &indices)
{
  //Make a brand new EventWorkspace
  EventWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create("EventWorkspace", 1, 2, 1));
  //Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(localworkspace, outputWorkspace, false);

  Progress progress(this,0,1, indices.size(), 1);

  //Get the pointer to the output event list
  EventList & outEL = outputWorkspace->getEventList(0);

  // Loop over spectra
  std::set<int>::iterator it;
  //for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  for (it = indices.begin(); it != indices.end(); it++)
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
