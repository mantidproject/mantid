#include "MantidVatesAPI/EscalatingRebinningActionManager.h"

namespace Mantid
{
namespace VATES
{

EscalatingRebinningActionManager::EscalatingRebinningActionManager() :
  m_currentAction(UseCache)
{
}
void EscalatingRebinningActionManager::ask(RebinningIterationAction requestedAction)
{
  //Very simply, only allow escalation if the requested action is more 'severe' than the current one.
  if (requestedAction > m_currentAction)
  {
    m_currentAction = requestedAction;
  }
}

RebinningIterationAction EscalatingRebinningActionManager::action() const
{
  return m_currentAction;
}

void EscalatingRebinningActionManager::reset()
{
  m_currentAction = UseCache;
}

EscalatingRebinningActionManager::~EscalatingRebinningActionManager()
{
}

}
}
