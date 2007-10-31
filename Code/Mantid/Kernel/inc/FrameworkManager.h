#ifndef FRAMEWORKMANAGER_H_
#define FRAMEWORKMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include <string>
#include <vector>
#include "Logger.h"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class IAlgorithm;
class Workspace;
//class AlgorithmFactory;
class AlgorithmManager;
class WorkspaceFactory;
class AnalysisDataService;

// N.B. Framework Manager is responsible for deleting the algorithms created
/** @class FrameworkManager FrameworkManager.h Kernel/FrameworkManager.h

    The main public API via which users interact with the Mantid framework.

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
    Copyright &copy; 2007 ???RAL???

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
  /// Default constructor
  FrameworkManager();
  /// Destructor
	virtual ~FrameworkManager();	
	
	/// Creates all of the required services
	void initialize();
	/// At the moment clears all memory associated with Algorithm Manager
	/// may do more in the future
	void clear();
	/** Creates an instance of an algorithm
	 * 
	 *  @param algName The name of the algorithm required
	 *  @return A pointer to the created algorithm
	 * 
	 *  @throw runtime_error Thrown if algorithm requested is not registered
	 */
	IAlgorithm* createAlgorithm(const std::string& algName);

  /** Creates an instance of an algorithm and sets the properties provided
   * 
   *  @param algName The name of the algorithm required
   *  @param propertiesArray A single string containing properties in the 
   *                         form "Property1:Value1,Property2:Value2,..."
   *  @return A pointer to the created algorithm
   * 
   *  @throw runtime_error Thrown if algorithm requested is not registered
   *                       or if properties string is ill-formed
   */	
	IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray);
	
	
  /** Creates an instance of an algorithm, sets the properties provided and
   *       then executes it.
   * 
   *  @param algName The name of the algorithm required
   *  @param propertiesArray A single string containing properties in the 
   *                         form "Property1:Value1,Property2:Value2,..."
   *  @return A pointer to the executed algorithm
   * 
   *  @throw runtime_error Thrown if algorithm requested is not registered,
   *                       if properties string is ill-formed or if
   *                       algorithm cannot be initialised or executed
   */ 
	IAlgorithm* exec(const std::string& algName, const std::string& propertiesArray);
	
	/** Returns a shared pointer to the workspace requested
	 * 
	 *  @param wsName The name of the workspace
	 *  @return A pointer to the workspace
	 * 
	 *  @throw runtime_error If workspace is not registered with analysis data service
	 */
	Workspace* getWorkspace(const std::string& wsName);
	
private:
  
  ///static reference to the logger class
  static Logger& g_log;
  
//the next lines will be removed and put in Algorithm Manager
  /// Pointer to the Algorithm Factory instance
  //AlgorithmFactory *algFactory;

  /// Pointer to the Algorithm Factory instance
  AlgorithmManager *algManager;


  /// Pointer to the Workspace Factory instance
  WorkspaceFactory *workFactory;
  /// Pointer to the Analysis Data Service
  AnalysisDataService *data;
	
};

} // Namespace Mantid

#endif /*FRAMEWORKMANAGER_H_*/
