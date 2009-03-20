#ifndef MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_
#define MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>

#include "MantidKernel/System.h"

namespace Mantid
{

namespace API
{
class IAlgorithm;
class Algorithm;
class MatrixWorkspace;
}

namespace PythonAPI
{

/** 
    FrameworkManager is a wrapper for the FrameworkManager class in Mantid. 
    As FrameworkManager is a singleton in Mantid it was easier to create a wrapper
    class to be used from python.

    @author ISIS, STFC
    @date 28/02/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
class DLLExport FrameworkManager
{
public:
	
	FrameworkManager();
	~FrameworkManager() {};

	// Clears all memory associated with the FrameworkManager 
	void clear();

	/// Clear memory associated with the AlgorithmManager
	void clearAlgorithms();
	
	/// Clear memory associated with the ADS
	void clearData();
	
	/// Clear memory associated with the ADS
	void clearInstruments();	

	// Creates and instance of an algorithm
	API::IAlgorithm* createAlgorithm(const std::string& algName);
	API::IAlgorithm* createAlgorithm(const std::string& algName, const int& version);

	// Creates an instance of an algorithm and sets the properties provided
	API::IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray);
	API::IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version);

	// Creates an instance of an algorithm, sets the properties provided & then executes it.
	API::IAlgorithm* execute(const std::string& algName, const std::string& propertiesArray);
	API::IAlgorithm* execute(const std::string& algName, const std::string& propertiesArray,const int& version);

	// Returns a pointer to the MatrixWorkspace requested
	API::MatrixWorkspace* getMatrixWorkspace(const std::string& wsName);

	// Returns a pointer to the TableWorkspace requested
	//DataObjects::TableWorkspace* getTableWorkspace(const std::string& wsName);

	// Deletes a workspace from the framework
	bool deleteWorkspace(const std::string& wsName);

	// Return the list of currently registered algorithm names
	std::vector<std::string> getAlgorithmNames() const;
	
	// Return the list of currently available workspace names
	std::vector<std::string> getWorkspaceNames() const;

	// Create the simple Python API for Mantid
	void createPythonSimpleAPI(bool);
	
	int addPythonAlgorithm(PyObject* pyAlg);
	void executePythonAlgorithm(std::string algName);

private:
	/// Copy constructor
	FrameworkManager(const FrameworkManager&);
	/// Assignment operator
	FrameworkManager& operator = (const FrameworkManager&);

};

}
}

#endif //MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_
