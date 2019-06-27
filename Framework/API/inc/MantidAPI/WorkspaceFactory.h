// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {
class ITableWorkspace;
class IPeaksWorkspace;
class Workspace;

/** The WorkspaceFactory class is in charge of the creation of all types
    of workspaces. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Laurent C Chapon, ISIS, RAL
    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007
*/

class MANTID_API_DLL WorkspaceFactoryImpl
    : public Kernel::DynamicFactory<Workspace> {
public:
  WorkspaceFactoryImpl(const WorkspaceFactoryImpl &) = delete;
  WorkspaceFactoryImpl &operator=(const WorkspaceFactoryImpl &) = delete;
  MatrixWorkspace_sptr create(const MatrixWorkspace_const_sptr &parent,
                              size_t NVectors = size_t(-1),
                              size_t XLength = size_t(-1),
                              size_t YLength = size_t(-1)) const;
  MatrixWorkspace_sptr create(const std::string &className,
                              const size_t &NVectors, const size_t &XLength,
                              const size_t &YLength) const;

  void initializeFromParent(const MatrixWorkspace &parent,
                            MatrixWorkspace &child,
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
  /// Private Destructor
  ~WorkspaceFactoryImpl() override = default;
  // Unhide the inherited create method but make it private
  using Kernel::DynamicFactory<Workspace>::create;
};

using WorkspaceFactory = Mantid::Kernel::SingletonHolder<WorkspaceFactoryImpl>;

template <class T, class... InitArgs>
boost::shared_ptr<T> createWorkspace(InitArgs... args) {
  auto ws = boost::make_shared<T>();
  ws->initialize(args...);
  return ws;
}

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::WorkspaceFactoryImpl>;
}
} // namespace Mantid

#endif /*MANTID_KERNEL_WORKSPACEFACTORY_H_*/
