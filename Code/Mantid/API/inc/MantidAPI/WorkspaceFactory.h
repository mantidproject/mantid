#ifndef MANTID_KERNEL_WORKSPACEFACTORY_H_
#define MANTID_KERNEL_WORKSPACEFACTORY_H_

/* Used to register classes into the factory. creates a global object in an 
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's 
 * subscribe method.
 */
#define DECLARE_WORKSPACE(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_ws_##classname( \
       ((Mantid::API::WorkspaceFactory::Instance()->subscribe<Mantid::DataObjects::classname>(#classname)) \
       , 0)); \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DynamicFactory.h"

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Workspace;

///shared pointer to the workspace base class
typedef boost::shared_ptr<Workspace> Workspace_sptr;

/** @class WorkspaceFactory WorkspaceFactory.h Kernel/WorkspaceFactory.h

    The WorkspaceFactory class is in charge of the creation of all types
    of workspaces. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Laurent C Chapon, ISIS, RAL
    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007
    
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
#ifdef IN_MANTID_API
DLLExport 
#else
DLLImport
#endif /* IN_MANTID_API */
WorkspaceFactory : public Kernel::DynamicFactory<Workspace>
{
public:
  
  // Returns the single instance of the factory
  static WorkspaceFactory* Instance();
  
  // Unhide the inherited create method
  using Kernel::DynamicFactory<Workspace>::create;
  
  Workspace_sptr create(const Workspace_sptr& parent) const;
  Workspace_sptr create(const std::string& className, const int& NVectors, 
                                   const int& XLength, const int& YLength) const;
  
private:
  
  // Private constructor and destructor for singleton class
  WorkspaceFactory();
  virtual ~WorkspaceFactory();
    
  /// Static reference to the logger class
  static Kernel::Logger& g_log;

  /// Pointer to the factory instance
  static WorkspaceFactory* m_instance;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_WORKSPACEFACTORY_H_*/
