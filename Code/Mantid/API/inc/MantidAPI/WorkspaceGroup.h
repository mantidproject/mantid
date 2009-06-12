#ifndef MANTID_DATAOBJECTS_WORKSPACEGROUP_H
#define MANTID_DATAOBJECTS_WORKSPACEGROUP_H
#include "MantidAPI/Workspace.h"
#include "Mantidkernel/DataService.h"
#include "MantidAPI\AnalysisDataService.h"
#include<string>
#include<map>
#include<iostream>


namespace Mantid
{
	namespace API
	{

		class DLLExport WorkspaceGroup:public Workspace,public Kernel::DataService<Workspace>
		{
			//friend ostream&os operator <<(ostream&os,const WorkspaceGroup& ws);
		public:
			WorkspaceGroup();
			~WorkspaceGroup();
			virtual const std::string id()const {return "WorkspaceGroup";} 
			virtual long int getMemorySize() const { return 0;} ;
			void add(const std::string& wsName);
			const std::vector<std::string>& getNames() const{return m_wsNames;}
			void print();
			void removeAll();
			void remove(const std::string& name);

		private:
			std::vector<std::string> m_wsNames;
			WorkspaceGroup(const WorkspaceGroup& ref);
			const WorkspaceGroup& operator=(const WorkspaceGroup&);
			static Kernel::Logger& g_log;
		};
		///shared pointer to the matrix workspace base class
typedef boost::shared_ptr<WorkspaceGroup> WorkspaceGroup_sptr;
///shared pointer to the matrix workspace base class (const version)
typedef boost::shared_ptr<const WorkspaceGroup> WorkspaceGroup_const_sptr;

	}
}
#endif