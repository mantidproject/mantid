#ifndef MANTID_KERNEL_WORKSPACEFACTORY_H_
#define MANTID_KERNEL_WORKSPACEFACTORY_H_

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_WORKSPACE(classname)                                           \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_ws_##classname(((Mantid::API::WorkspaceFactory::Instance()      \
                                    .subscribe<classname>(#classname)),        \
                               0));                                            \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class ITableWorkspace;
class IPeaksWorkspace;

/** The WorkspaceFactory class is in charge of the creation of all types
    of workspaces. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Laurent C Chapon, ISIS, RAL
    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_API_DLL WorkspaceFactoryImpl
    : public Kernel::DynamicFactory<Workspace> {
public:
  MatrixWorkspace_sptr create(const MatrixWorkspace_const_sptr &parent,
                              size_t NVectors = size_t(-1),
                              size_t XLength = size_t(-1),
                              size_t YLength = size_t(-1)) const;
  MatrixWorkspace_sptr create(const std::string &className,
                              const size_t &NVectors, const size_t &XLength,
                              const size_t &YLength) const;

  void initializeFromParent(const MatrixWorkspace_const_sptr parent,
                            const MatrixWorkspace_sptr child,
                            const bool differentSize) const;

  /// Create a ITableWorkspace
  boost::shared_ptr<ITableWorkspace>
  createTable(const std::string &className = "TableWorkspace") const;

  /// Create a IPeaksWorkspace
  boost::shared_ptr<IPeaksWorkspace>
  createPeaks(const std::string &className = "PeaksWorkspace") const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<WorkspaceFactoryImpl>;

  /// Private Constructor for singleton class
  WorkspaceFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  WorkspaceFactoryImpl(const WorkspaceFactoryImpl &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  WorkspaceFactoryImpl &operator=(const WorkspaceFactoryImpl &);
  /// Private Destructor
  virtual ~WorkspaceFactoryImpl();

  // Unhide the inherited create method but make it private
  using Kernel::DynamicFactory<Workspace>::create;
};

/// Forward declaration of a specialisation of SingletonHolder for
/// AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<WorkspaceFactoryImpl>;
#endif /* _WIN32 */
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<WorkspaceFactoryImpl>
    WorkspaceFactory;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_WORKSPACEFACTORY_H_*/
