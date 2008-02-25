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
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class IAlgorithm;
class Workspace;

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
class EXPORT_OPT_MANTID_API FrameworkManagerImpl
{
public:
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
	friend struct Mantid::Kernel::CreateUsingNew<FrameworkManagerImpl>;
	
	///Private Constructor
	FrameworkManagerImpl();
	virtual ~FrameworkManagerImpl();	
	/// Private copy constructor - NO COPY ALLOWED
	FrameworkManagerImpl(const FrameworkManagerImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	FrameworkManagerImpl& operator = (const FrameworkManagerImpl&);
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  
};

	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FrameworkManagerImpl>;
#endif /* _WIN32 */
	typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FrameworkManagerImpl> FrameworkManager;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FRAMEWORKMANAGER_H_*/
