#include "MantidAPI/MultiPeriodGroupWorker.h"

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MultiPeriodGroupWorker::MultiPeriodGroupWorker()
  {
  }

  /**
   * Constructor
   * @param workspacePropertyName : Property name to treat as source of multiperiod workspaces.
   */
  MultiPeriodGroupWorker::MultiPeriodGroupWorker(const std::string& workspacePropertyName) : m_workspacePropertyName(workspacePropertyName)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultiPeriodGroupWorker::~MultiPeriodGroupWorker()
  {
  }
  

  bool MultiPeriodGroupWorker::checkGroups(IAlgorithm_sptr alg) const
  {
    return false;
  }

  bool MultiPeriodGroupWorker::useCustomWorkspaceProperty() const
  {
    return !this->m_workspacePropertyName.empty();
  }


} // namespace API
} // namespace Mantid
