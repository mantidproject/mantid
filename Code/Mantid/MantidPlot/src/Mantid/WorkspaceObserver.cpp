//-----------------------------------
// Includes
//-----------------------------------
#include "WorkspaceObserver.h"

//------------------------------------ ObserverCallback class -------------------------------

void ObserverCallback::handleDelete(const std::string &name, boost::shared_ptr<Mantid::API::Workspace> workspace)
{
  m_observer->deleteHandle(name, workspace);
  
}

void ObserverCallback::handleAfterReplace(const std::string &name, boost::shared_ptr<Mantid::API::Workspace> workspace)
{
  m_observer->afterReplaceHandle(name, workspace);
}

void ObserverCallback::handleClearADS()
{
  m_observer->clearADSHandle();
}
