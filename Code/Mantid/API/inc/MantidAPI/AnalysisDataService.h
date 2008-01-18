#ifndef MANTID_KERNEL_ANALYSISDATASERVICE_H_
#define MANTID_KERNEL_ANALYSISDATASERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"

#include <string>
#include <map>

namespace Mantid
{
namespace API
{
/** @class AnalysisDataService AnalysisDataService.h Kernel/AnalysisDataService.h

    The Analysis data service stores instances of the Workspace objects and 
    anything that derives from template class DynamicFactory<Mantid::Kernel::IAlgorithm>.  
    This is the primary data service that
    the users will interact with either through writing scripts or directly
    through the API. It is implemented as a singleton class.

    This is the manager/owner of Workspace* when registered.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 01/10/2007
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport AnalysisDataService
{
public:
   
  // Returns the single instance of the service
  static AnalysisDataService* Instance();
  
  void add( const std::string& name, Workspace_sptr space );
  void addOrReplace( const std::string& name, Workspace_sptr space);
  void remove( const std::string& name );
  Workspace_sptr retrieve( const std::string& name );	
	
private:
  
  // Private constructors and destructor for singleton class
  AnalysisDataService();
  AnalysisDataService(const AnalysisDataService&);
  ~AnalysisDataService();

  ///static reference to the logger class
  static Kernel::Logger& g_log;
  
  /// Pointer to the single instance
  static AnalysisDataService* m_instance;
  
  /// Typedef for the map of the managed algorithms and their names
  typedef std::map<std::string, Workspace_sptr> WorkspaceMap;
  /// The map holding the managed algorithms
  WorkspaceMap m_spaces;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ANALYSISDATASERVICE_H_*/
