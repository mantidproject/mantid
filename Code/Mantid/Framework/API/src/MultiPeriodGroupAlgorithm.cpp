#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {
    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    MultiPeriodGroupAlgorithm::MultiPeriodGroupAlgorithm() :  m_worker(new MultiPeriodGroupWorker)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    MultiPeriodGroupAlgorithm::~MultiPeriodGroupAlgorithm()
    {
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
      if(this->useCustomInputPropertyName())
      {
        const std::string propName = this->fetchInputPropertyName();
        m_worker.reset(new MultiPeriodGroupWorker(propName));
      }
      m_multiPeriodGroups = m_worker->findMultiPeriodGroups(this);
      bool useDefaultGroupingBehaviour = m_multiPeriodGroups.size() == 0;
      /*
       * Give the opportunity to treat this as a regular group workspace.
       */
      if(useDefaultGroupingBehaviour)
      {
        return Algorithm::checkGroups(); // Delegate to algorithm base class.
      }
      else
      {
        return !useDefaultGroupingBehaviour; // Evaluates to True if if multiperiod, that way algorithm will call the overrriden processGroups.
      }
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
      bool result = m_worker->processGroups(this, m_multiPeriodGroups);
      /*
       * If we could not process the groups as a multiperiod set of groups workspaces
       */
      if(!result)
      {
        result = Algorithm::processGroups();
      }

      this->setExecuted(result);
      return result;
    }

  } // namespace API
} // namespace Mantid
