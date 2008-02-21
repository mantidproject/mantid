#ifndef MANTID_KERNEL_FRAMEWORKMANAGER_H_
#define MANTID_KERNEL_FRAMEWORKMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Kernel
{
class ConfigService;
}
}

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class IAlgorithm;
class Workspace;
class WorkspaceFactory;
class AnalysisDataService;

/** @class FrameworkManager FrameworkManager.h Kernel/FrameworkManager.h

    The main public API via which users interact with the Mantid framework.

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
    
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
class
#if IN_MANTID_API
DLLExport 
#else
DLLImport
#endif
FrameworkManager
{
public:
	
  static FrameworkManager* Instance();

  virtual ~FrameworkManager();	

/// Creates all of the required services
  void initialize();

  /// Clears all memory associated with the AlgorithmManager 
  void clear();

  /// Creates and instance of an algorithm
  IAlgorithm* createAlgorithm(const std::string& algName);

  /// Creates an instance of an algorithm and sets the properties provided
	IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray);
		
  /// Creates an instance of an algorithm, sets the properties provided & then executes it.
	IAlgorithm* exec(const std::string& algName, const std::string& propertiesArray);
	
	/// Returns a shared pointer to the workspace requested
	Workspace* getWorkspace(const std::string& wsName);
	
private:
	
 ///Private Constructor
	FrameworkManager();
	/// Private copy constructor - NO COPY ALLOWED
	FrameworkManager(const FrameworkManager&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	FrameworkManager& operator = (const FrameworkManager&);
  
  /// Pointer to the instance
  static FrameworkManager* m_instance;
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
 
  /// Pointer to the Workspace Factory instance
  WorkspaceFactory *workFactory;
  /// Pointer to the Analysis Data Service
  AnalysisDataService *data;
  /// Pointer to the Configuration Service
  Kernel::ConfigService *config;
  /// Pointer to the Library Manager
  Kernel::LibraryManager *libManager;
  
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FRAMEWORKMANAGER_H_*/
