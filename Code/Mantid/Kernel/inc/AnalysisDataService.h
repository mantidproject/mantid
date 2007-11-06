#ifndef MANTID_KERNEL_ANALYSISDATASERVICE_H_
#define MANTID_KERNEL_ANALYSISDATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include "StatusCode.h"
#include "Workspace.h"
#include "Logger.h"

#include <string>
#include <map>

namespace Mantid
{
namespace Kernel
{
/** @class AnalysisDataService AnalysisDataService.h Kernel/AnalysisDataService.h

    The Analysis data service stores instances of the Workspace objects and 
    anything that derives from them.  This is the primary data service that
    the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    
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
class DLLExport AnalysisDataService
{
public:
  
  /** Static method which retrieves the single instance of the Analysis data service
   * 
   *  @returns A pointer to the service instance
   */
  static AnalysisDataService* Instance();
  
	/** Add a pointer to a named workspace to the data service store.
	 *  Upon addition, the data service assumes ownership of the workspace.
	 * 
	 *  @param name The user-given name for the workspace
	 *  @param space A pointer to the workspace
	 *  @return A StatusCode object indicating whether the operation was successful
	 */
	StatusCode add( std::string name, Workspace * space );
	
	/** Remove a workspace from the data service store.
	 *  Upon removal, the workspace itself will be deleted.
	 * 
	 *  @param name The user-given name for the workspace
	 *  @return A StatusCode object indicating whether the operation was successful
	 */
	StatusCode remove( std::string name );
	
	/** Retrieve a pointer to a workspace by name.
	 * 
	 *  @param name The name of the desired workspace
	 *  @param space Returns a pointer to the requested workspace
	 *  @return A StatusCode object indicating whether the operation was successful
	 */
	StatusCode retrieve( std::string name, Workspace *& space );
	
	
private:
  
  /// Private Constructor for singleton class
  AnalysisDataService();
  
  /** Private copy constructor
   *  Prevents singleton being copied
   */
  AnalysisDataService(const AnalysisDataService&) {}
  
  /** Private destructor
   *  Prevents client from calling 'delete' on the pointer handed 
   *  out by Instance
   */
  ~AnalysisDataService();

  
  ///static reference to the logger class
  static Logger& g_log;
  
  /// Pointer to the single instance
  static AnalysisDataService* m_instance;
  
  /// Typedef for the map of the managed algorithms and their names
  typedef std::map<std::string, Workspace *> WorkspaceMap;
  /// The map holding the managed algorithms
  WorkspaceMap * m_spaces;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICE_H_*/
