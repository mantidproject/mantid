#ifndef FRAMEWORKMANAGER_H_
#define FRAMEWORKMANAGER_H_

#include <string>

//#include "IAlgorithm.h"
//#include "AlgorithmFactory.h"
//#include "WorkspaceFactory.h"
//#include "AnalysisDataService.h"

namespace Mantid
{

class IAlgorithm;
class Workspace;
class AlgorithmFactory;
class WorkspaceFactory;
class AnalysisDataService;

// N.B. Framework Manager is responsible for deleting the algorithms created

class FrameworkManager
{
public:
	FrameworkManager();
	virtual ~FrameworkManager();
	
	// create all of the required services etc
	void initialize();
	
	// use the algorithm factory to create an instance of an algorithm
	IAlgorithm* createAlgorithm(std::string algName);

	// as above but use the array of strings to set algorithm properties
	// RJT: not sure whether this is the right form for the second argument
	IAlgorithm* createAlgorithm(std::string algName, std::string propertiesArray);
	
	// as above but also execute the algorithm
	IAlgorithm* exec(std::string algName, std::string propertiesArray);
	
	// returns a shared pointer to the workspace requested
	Workspace* getWorkspace(std::string wsName);
	
private:
  AlgorithmFactory *algFactory;
  WorkspaceFactory *workFactory;
  AnalysisDataService *data;
	
};

}

#endif /*FRAMEWORKMANAGER_H_*/
