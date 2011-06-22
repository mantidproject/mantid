//-----------------------------------
// Includes
//-----------------------------------
#include "MantidQtAPI/WorkspaceObserver.h"

namespace MantidQt
{
namespace API
{

//------------------------------------ ObserverCallback class -------------------------------

void ObserverCallback::handleDelete(const std::string &name, boost::shared_ptr<Mantid::API::Workspace> workspace)
{
  m_observer->deleteHandle(name, workspace);
}

void ObserverCallback::handleAdd(const std::string &name, boost::shared_ptr<Mantid::API::Workspace> workspace)
{
  m_observer->addHandle(name, workspace);
}

void ObserverCallback::handleAfterReplace(const std::string &name, boost::shared_ptr<Mantid::API::Workspace> workspace)
{
  m_observer->afterReplaceHandle(name, workspace);
}

void ObserverCallback::handleClearADS()
{
  m_observer->clearADSHandle();
}

} // MantidQt
} // API