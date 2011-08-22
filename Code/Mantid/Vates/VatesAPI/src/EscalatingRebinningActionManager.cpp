#include "MantidVatesAPI/EscalatingRebinningActionManager.h"

namespace Mantid
{
namespace VATES
{


/*
Constructor
@param action : optional starting action.
*/
EscalatingRebinningActionManager::EscalatingRebinningActionManager(RebinningIterationAction action) :
  m_currentAction(action)
{
}

 /** Request that some level of action is peformed.
 * @param requestedAction */
void EscalatingRebinningActionManager::ask(RebinningIterationAction requestedAction)
{
  //Very simply, only allow escalation if the requested action is more 'severe' than the current one.
  if (requestedAction > m_currentAction)
  {
    m_currentAction = requestedAction;
  }
}

/** Get the selected action.
 * @return the selected action
 */
RebinningIterationAction EscalatingRebinningActionManager::action() const
{
  return m_currentAction;
}

///Reset the escalation path to the minimum level.
void EscalatingRebinningActionManager::reset()
{
  m_currentAction = UseCache;
}

///Destructor
EscalatingRebinningActionManager::~EscalatingRebinningActionManager()
{
}

}
}
