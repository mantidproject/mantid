#ifndef WORKSPACEFACTORY_H_
#define WORKSPACEFACTORY_H_

#include <map>
#include <vector> 

namespace Mantid
{

// Forward declaration
class Workspace;

/** @class WorkspaceFactory WorkspaceFactory.h
 	
 	    Concrete factory for Workspaces. 
 			Requirement : Implemented as a singleton
 			Workspaces types identified by strings. 
 			A workspace of type s is created calling
 			CreateWorkspace(s)
 	    @author Laurent C Chapon, ISIS, RAL
	    @date 26/09/2007
 	    
 	    Copyright © 2007 ???RAL???
 	
 	    This file is part of Mantid.
 	
 	    Mantid is free software; you can redistribute it and/or modify
 	    it under the terms of the GNU General Public License as published by
 	    the Free Software Foundation; either version 3 of the License, or
 	    (at your option) any later version.
 	
 	    Mantid is distributed in the hope that it will be useful,
 	    but WITHOUT ANY WARRANTY; without even the implied warranty of
 	    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 	    GNU General Public License for more details.
 	
 	    You should have received a copy of the GNU General Public License
 	    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 	
 	    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 	*/
class WorkspaceFactory
{
public:
  // Typedef for pointer to callback.
  typedef Workspace* (*CreateWorkspaceCallback)();
  //Get the singleton Instance.
	static WorkspaceFactory* Instance();
	// Method to call to create a workspace.
	// Throws if string is empty.
	// Throws if workspace type is not registered with the factory.
	Workspace* createWorkspace(const std::string&) const;
	// Method to call to register a Workspace type.
	// Throws if sting is empty.
	// If a type is already registered it will erase the old callback
	// and replace with current parameters.
	// Return false if insertion in the callbackmap fails.
	bool registerWorkspace(const std::string&, CreateWorkspaceCallback);
	// Method to call to unregister a Workspace. 
	// Return false if workspace type not found.
	bool unregisterWorkspace(const std::string&);
private:
  //
	WorkspaceFactory();
	WorkspaceFactory(const WorkspaceFactory&);
	WorkspaceFactory& operator=(const WorkspaceFactory&);
	~WorkspaceFactory();
	typedef std::map<std::string,CreateWorkspaceCallback> w_map;
	// singleton instance 
	static WorkspaceFactory* _instance;
	// map of callback for object creation
	w_map _workmap;
};


}
#endif /*WORKSPACEFACTORY_H_*/
