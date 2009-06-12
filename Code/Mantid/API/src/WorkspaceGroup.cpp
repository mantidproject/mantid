#include "MantidAPI\WorkspaceGroup.h"
//#include<iostream>

Mantid::Kernel::Logger& Mantid::API::WorkspaceGroup::g_log=Mantid::Kernel::Logger::get("WorkspaceGroup");
namespace Mantid
{
	namespace API
	{
		WorkspaceGroup::WorkspaceGroup(): Kernel::DataService<Workspace>("Workspace")
		{
		}
		WorkspaceGroup::~WorkspaceGroup()
		{
			//g_log.error()<<"WorkspaceGroup desctructor called "<<std::endl;
			//std::cout<<"WorkspaceGroup desctructor called "<<std::endl;
		}
		void WorkspaceGroup::add(const std::string& name)
		{			
			m_wsNames.push_back(name);
			//g_log.error()<<"workspacename added to group vector=  "<<name<<std::endl;
		}
		void WorkspaceGroup::removeAll()
		{
			//m_wsNames.erase(m_wsNames.begin(),m_wsNames.end());
			m_wsNames.clear();
		}
		void WorkspaceGroup::remove(const std::string& name)
		{
			std::vector<std::string>::iterator itr;
			for(itr=m_wsNames.begin();itr!=m_wsNames.end();itr++)
			{
				if((*itr)==name)
					m_wsNames.erase(itr);
				else
					g_log.warning("Workspace  "+name+"not found in workspacegroup");
			}
		}
		void WorkspaceGroup::print()
		{
			std::vector<std::string>::const_iterator itr;
			for(itr=m_wsNames.begin();itr!=m_wsNames.end();itr++)
			{
				g_log.error()<<"workspacename in group vector=  "<<*itr<<std::endl;
			}
		}

	}
}