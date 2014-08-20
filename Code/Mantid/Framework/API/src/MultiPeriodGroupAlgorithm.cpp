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
    MultiPeriodGroupAlgorithm::MultiPeriodGroupAlgorithm() : m_useDefaultGroupingBehaviour(true)
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

      m_useDefaultGroupingBehaviour = m_multiPeriodGroups.size() == 0;
      return !m_useDefaultGroupingBehaviour;
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

      this->setExecuted(result);
      return result;
    }

  } // namespace API
} // namespace Mantid
