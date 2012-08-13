#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MultiPeriodGroupAlgorithm::MultiPeriodGroupAlgorithm() : m_useDefaultGroupingBehaviour(true) 
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultiPeriodGroupAlgorithm::~MultiPeriodGroupAlgorithm()
  {
  }
  
  /**
Validate the multiperiods workspace groups. Gives the opportunity to exit processing if things don't look right.
@input nInputWorkspaces: Number of input workspaces.
*/
void MultiPeriodGroupAlgorithm::validateMultiPeriodGroupInputs(const size_t& nInputWorkspaces) const
{
  const size_t multiPeriodGroupsSize = m_multiPeriodGroups.size();
  if(multiPeriodGroupsSize != 0 && multiPeriodGroupsSize != nInputWorkspaces)
  {
    std::string msg = "MergeRuns can either process complete array of MatrixWorkspaces or Multi-period-groups, but mixing of types is not permitted.";
    throw std::runtime_error(msg);
  }

  if(multiPeriodGroupsSize > 0)
  { 
    const size_t benchMarkGroupSize = m_multiPeriodGroups[0]->size();
    for(size_t i = 0; i < multiPeriodGroupsSize; ++i)
    {
      WorkspaceGroup_sptr currentGroup = m_multiPeriodGroups[i];
      if(currentGroup->size() != benchMarkGroupSize)
      {
       throw std::runtime_error("Not all the input Multi-period-group input workspaces are the same size.");
      }
      for(size_t j = 0; j < currentGroup->size(); ++j)
      {
        MatrixWorkspace_const_sptr currentNestedWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(currentGroup->getItem(j));
        Property* nPeriodsProperty = currentNestedWS->run().getLogData("nperiods");
        size_t nPeriods = atoi(nPeriodsProperty->value().c_str());
        if(nPeriods != benchMarkGroupSize)
        {
          throw std::runtime_error("Missmatch between nperiods log and the number of workspaces in the input group: " + m_multiPeriodGroups[i]->name());
        }
        Property* currentPeriodProperty = currentNestedWS->run().getLogData("current_period");
        size_t currentPeriod = atoi(currentPeriodProperty->value().c_str());
        if(currentPeriod != (j+1))
        {
          throw std::runtime_error("Multiperiod group workspaces must be ordered by current_period. Correct: " + currentNestedWS->name());
        }
      }
    }
  }
}

/** Check the input workspace properties for groups.
*
* Overriden from base Algorithm class.
*
* Checks to see if the inputs are MULTIPERIOD group data.
*
* @throw std::invalid_argument if the groups sizes are incompatible.
* @throw std::invalid_argument if a member is not found
*
* This method (or an override) must NOT THROW any exception if there are no input workspace groups
*/
bool MultiPeriodGroupAlgorithm::checkGroups()
{
  typedef std::vector<std::string> WorkspaceNameType;

  m_multiPeriodGroups.clear();
  WorkspaceNameType workspaces = this->getProperty("InputWorkspaces");
  WorkspaceNameType::iterator it = workspaces.begin();

  // Inspect all the input workspaces in the ArrayProperty input.
  while(it != workspaces.end())
  {
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve(*it);
    if(!ws)
    {
      throw Kernel::Exception::NotFoundError("Workspace", *it);
    }
    WorkspaceGroup_sptr inputGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if(inputGroup)
    {
      if(inputGroup->isMultiperiod())
      {
        m_multiPeriodGroups.push_back(inputGroup);
      }
    }
    ++it;
  }
  const size_t multiPeriodGroupsSize = m_multiPeriodGroups.size();
  // If there are no MULTIPERIOD group workpaces detected, we hand the checking back up toe the base class.
  if(multiPeriodGroupsSize == 0)
  {
    // This will prevent (this) implementation of processGroups from being run. The base class proccessGroups will be used instead.
    m_useDefaultGroupingBehaviour = true;
    // Use the base class inmplementation.
    return Algorithm::checkGroups(); 
  }
  // Check that we have correct looking group workspace indexes.
  validateMultiPeriodGroupInputs(workspaces.size());

  m_useDefaultGroupingBehaviour = false;
  return !m_useDefaultGroupingBehaviour;
}


/**
Creates a list of input workspaces as a string for a given period using all nested workspaces at that period 
within all group workspaces.

This requires a little explanation, because this is the reason that this algorithm needs a customised overriden checkGroups and processGroups
method:

Say you have two multiperiod group workspaces A and B and an output workspace C. A contains matrix workspaces A_1 and A_2, and B contains matrix workspaces B_1 and B2. Because this
is multiperiod data. A_1 and B_1 share the same period, as do A_2 and B_2. So merging must be with respect to workspaces of equivalent periods. Therefore,
merging must be A_1 + B_1 = C_1 and A_2 + B_2 = C_2. This method constructs the inputs for a nested call to MergeRuns in this manner.

@param periodIndex : zero based index denoting the period.
@return comma separated string of input workspaces.
*/
std::string MultiPeriodGroupAlgorithm::createFormattedInputWorkspaceNames(const size_t& periodIndex) const
{
  std::string prefix = "";
  std::string inputWorkspaces = "";
  for(size_t j = 0; j < m_multiPeriodGroups.size(); ++j)
  {
    inputWorkspaces += prefix + m_multiPeriodGroups[j]->getItem(periodIndex)->name();
    prefix = ",";
  }
  return inputWorkspaces;
}

//--------------------------------------------------------------------------------------------
/** Process WorkspaceGroup inputs.
*
* Overriden from Algorithm base class.
*
* This should be called after checkGroups(), which sets up required members.
* It goes through each member of the group(s), creates and sets an algorithm
* for each and executes them one by one.
*
* If there are several group input workspaces, then the member of each group
* is executed pair-wise.
*
* @return true - if all the workspace members are executed.
*/
bool MultiPeriodGroupAlgorithm::processGroups()
{
  // If we are not processing multiperiod groups, use the base behaviour.
  if(m_useDefaultGroupingBehaviour)
  {
    return Algorithm::processGroups();
  }

  Property* outputWorkspaceProperty = this->getProperty("OutputWorkspace");
  const std::string outName = outputWorkspaceProperty->value();

  size_t nPeriods = m_multiPeriodGroups[0]->size();
  const bool doObserveADSNotifications = true;
  WorkspaceGroup_sptr outputWS = boost::make_shared<WorkspaceGroup>(!doObserveADSNotifications);
  // Loop through all the periods.
  for(size_t i = 0; i < nPeriods; ++i)
  {
    // Create a formatted input workspace list. As this is the usual input (ArrayProperty) to the MergeRuns algorithm.
    const std::string inputWorkspaces = createFormattedInputWorkspaceNames(i);

    Algorithm_sptr alg_sptr = API::AlgorithmManager::Instance().createUnmanaged(this->name(), this->version());
    IAlgorithm* alg = alg_sptr.get();
    if(!alg)
    {
      g_log.error()<<"CreateAlgorithm failed for "<<this->name()<<"("<<this->version()<<")"<<std::endl;
      throw std::runtime_error("Algorithm creation failed.");
    }
    alg->initialize();
    alg->setPropertyValue("InputWorkspaces", inputWorkspaces);
    // Create a name for the output workspace based upon the requested name for the overall output group workspace.
    const std::string outName_i = outName + "_" + Strings::toString(i+1);
    alg->setPropertyValue("OutputWorkspace", outName_i);

    if (!alg->execute())
    {
      throw std::runtime_error("Execution of " + this->name() + " for group entry " + Strings::toString(i+1) + " failed.");
    }
    outputWS->add(outName_i);
  }
  outputWS->observeADSNotifications(doObserveADSNotifications);
  this->setProperty("OutputWorkspace", outputWS);
  this->setExecuted(true);
  AnalysisDataService::Instance().addOrReplace(outName, outputWS);
  return true;
}

} // namespace API
} // namespace Mantid