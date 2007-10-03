#include "../inc/WorkspaceFactory.h"
#include <stdexcept>
#include <algorithm> 

namespace Mantid
{

WorkspaceFactory* WorkspaceFactory::_instance=0;

WorkspaceFactory::WorkspaceFactory()
{
	
}

WorkspaceFactory::~WorkspaceFactory()
{
	delete _instance;
}

WorkspaceFactory* WorkspaceFactory::Instance()
{
	if (!_instance) _instance=new WorkspaceFactory;
	return _instance;
}

Workspace* WorkspaceFactory::createWorkspace(const std::string& rhs) const
{
	if (rhs.empty()) throw std::runtime_error("WorkspaceFactory::CreateWorkspace, empty string");
	std::map<std::string,CreateWorkspaceCallback>::const_iterator it=_workmap.find(rhs);
	if (it==_workmap.end())
		throw std::runtime_error("WorkspaceFactory::CreateWorkspace, workspace type not known");
	return (it->second)();
}

bool WorkspaceFactory::registerWorkspace(const std::string& s, CreateWorkspaceCallback c)
{
	if (s.empty()) throw std::runtime_error("WorkspaceFactory::RegisterWorkspace, empty string");
	std::map<std::string,CreateWorkspaceCallback>::iterator it=_workmap.find(s);
	if (it!=_workmap.end()) //Workspace type already registered
	{
		_workmap.erase(it);
	}
	return _workmap.insert(std::pair<std::string,CreateWorkspaceCallback>(s,c)).second;
}

bool WorkspaceFactory::unregisterWorkspace(const std::string& s)
{
		return _workmap.erase(s)==1;
}

} // Namespace Mantid
