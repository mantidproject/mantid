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
#include "MantidKernel/make_unique.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace DataObjects {
class EventWorkspace;
}
namespace API {
class HistoWorkspace;
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
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
  WorkspaceFactoryImpl(const WorkspaceFactoryImpl &) = delete;
  WorkspaceFactoryImpl &operator=(const WorkspaceFactoryImpl &) = delete;
  MatrixWorkspace_sptr create(const MatrixWorkspace_const_sptr &parent,
                              size_t NVectors = size_t(-1),
                              size_t XLength = size_t(-1),
                              size_t YLength = size_t(-1)) const;
  MatrixWorkspace_sptr create(const std::string &className,
                              const size_t &NVectors, const size_t &XLength,
                              const size_t &YLength) const;
  MatrixWorkspace_sptr createNoInit(const std::string &className) const;

  void initializeFromParent(const MatrixWorkspace_const_sptr &parent,
                            const MatrixWorkspace_sptr &child,
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

typedef Mantid::Kernel::SingletonHolder<WorkspaceFactoryImpl> WorkspaceFactory;

template <class T, class... InitArgs>
boost::shared_ptr<T> createWorkspace(InitArgs... args) {
  auto ws = boost::make_shared<T>();
  ws->initialize(args...);
  return ws;
}



/*
// Create from parent, same spectra, same bin edges
auto ws = create<HistoWorkspace>(parentWS);

// Create from parent, same spectra
auto ws = create<HistoWorkspace>(parentWS, edges);

// Create from parent, same spectra
auto ws = create<HistoWorkspace>(parentWS, edges, counts);

// Create from parent, extract some spectrum numbers
// Option 1 (verbose):
IndexTranslator translator(parentWS.indexTranslator(), spectrumNumbers);
auto ws = create<HistoWorkspace>(parentWS, translator);
// Option 2 (unclear notation):
// Are spectrumNumbers arbitrary and new, or to be extracted from parentWS?
auto ws = create<HistoWorkspace>(parentWS, spectrumNumbers);

// rename IndexTranslator -> Indexer?

template <class T, class... Args>
std::unique_ptr<T> create(const MatrixWorkspace &parent, Args &&... args) {
  // no translator, copy from parent
  create<T>(parent, parent.indexTranslator(), std::forward<Args>(args)...);
}

template <class T, class... Args>
std::unique_ptr<T> create(const MatrixWorkspace &parent,
                          const IndexTranslator &translator, Args &&... args) {
  // 1. Figure out (dynamic) target type:
  // - Type is same as parent if T is base of parent
  // - If T is not base of parent, conversion may occur. Currently only
  //   supported for EventWorkspace
  std::string id(parent->id());
  if (std::is_base_of<HistoWorkspace, T>::value && id == "EventWorkspace") {
    // drop events
    // create Workspace2D or T, whichever is more derived?
    // if T is more derived than Workspace2D there must be an error?
    id = "Workspace2D";
  }

  auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create(id));
  if (!ws)
    throw std::runtime_error("Invalid conversion across workspace type hierarchy");

  // args are used to construct histogram for initialization
  if (sizeof...(args) == 0)
    ws->initialize(translator, parent.x(0).size(), parent.blocksize());
  else
    ws->initialize(translator, std::forward<Args>(args)...);
  WorkspaceFactory::Instance().initializeFromParent(parent, ws);

}

// TODO do not use sptr as argument
template <class T>
std::unique_ptr<T> create(const MatrixWorkspace_const_sptr &parent,
                          const IndexTranslator &translator, // sets new spectrum numbers and spectrum-detector associations
                          const BinEdges &edges) {}
template <class T>
std::unique_ptr<T> create(const MatrixWorkspace_const_sptr &parent,
                          const std::vector<SpectrumNumber> &spectrumNumbers, // extract these spectra from parent
                          const BinEdges &edges) {}
                          */


} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::WorkspaceFactoryImpl>;
}
}

namespace Mantid {
namespace API {

//template <class T, class P, class... Args>
//std::unique_ptr<T> create(const P &parent, Args &&... args) {
//  return create<T>(parent, parent.getNumberHistograms(),
//                   std::forward<Args>(args)...);
//}

  /*
template <class T, class P>
void initializeHelper(const P &parent, T &ws) {
  if (ws.id() == "EventWorkspace")
    ws.initialize(parent.getNumberHistograms(), parent.blocksize() + 1,
                  parent.blocksize());
  else
    ws.initialize(parent.getNumberHistograms(), parent.x(0).size(),
                  parent.blocksize());
}

template <class T, class P, class Size>
void initializeHelper(const P &parent, T &ws, Size size) {
  if (ws.id() == "EventWorkspace")
    ws.initialize(size, parent.blocksize() + 1, parent.blocksize());
  else
    ws.initialize(size, parent.x(0).size(), parent.blocksize());
}

template <class T, class P, class... Args>
void initializeHelper(const P &parent, T &ws, Args &&... args) {
  ws.initialize(std::forward<Args>(args)...);
}

template <class T, class P, class... Args>
boost::shared_ptr<T> create(const P &parent, Args &&... args) {
  // 1. Figure out (dynamic) target type:
  // - Type is same as parent if T is base of parent
  // - If T is not base of parent, conversion may occur. Currently only
  //   supported for EventWorkspace
  std::string id(parent->id());
  if (std::is_base_of<HistoWorkspace, T>::value && id == "EventWorkspace") {
    // drop events
    // create Workspace2D or T, whichever is more derived?
    // if T is more derived than Workspace2D there must be an error?
    id = "Workspace2D";
  }

  auto ws = boost::dynamic_pointer_cast<T>(
      WorkspaceFactory::Instance().createNoInit(id));
  if (!ws)
    throw std::runtime_error(
        "Invalid conversion across workspace type hierarchy");

  // args are used to construct histogram for initialization
  initializeHelper(*parent, *ws, std::forward<Args>(args)...);
  WorkspaceFactory::Instance().initializeFromParent(
      parent, ws, parent->y(0).size() != ws->y(0).size());

  return ws;
}
*/

/*
template <class T>
HistogramData::Histogram createEmptyHistogramFor(const HistogramData::Histogram &parent) {
  return parent;
}

template <>
HistogramData::Histogram
createEmptyHistogramFor<DataObjects::EventWorkspace>(const HistogramData::Histogram &parent) {
  return HistogramData::Histogram(parent.binEdges());
}
*/

/*
namespace detail {
MANTID_API_DLL HistogramData::Histogram
stripData(HistogramData::Histogram histogram);
}

template <class T, class P> boost::shared_ptr<T> create(const P &parent) {
  // copy X??
  return create<T>(parent, detail::stripData(parent->histogram(0)));
}

template <class T, class P>
boost::shared_ptr<T> create(const P &parent, const size_t numSpectra) {
  return create<T>(parent, numSpectra, detail::stripData(parent->histogram(0)));
}

template <class T, class P, class... HistArgs>
boost::shared_ptr<T> create(const P &parent, HistArgs &&... histArgs) {
  return create<T>(parent, parent->getNumberHistograms(),
                   std::forward<HistArgs>(histArgs)...);
}

template <class T, class P, class... HistArgs>
boost::shared_ptr<T> create(const P &parent, const size_t numSpectra,
                            HistArgs &&... histArgs) {
  // 1. Figure out (dynamic) target type:
  // - Type is same as parent if T is base of parent
  // - If T is not base of parent, conversion may occur. Currently only
  //   supported for EventWorkspace
  std::unique_ptr<T> ws;
  if (std::is_base_of<HistoWorkspace, T>::value &&
      parent->id() == "EventWorkspace") {
    // drop events
    // create Workspace2D or T, whichever is more derived?
    // if T is more derived than Workspace2D there must be an error?
    ws = Kernel::make_unique<DataObjects::Workspace2D>();
  } else {
    // This may throw std::bad_cast.
    ws = dynamic_cast<const T &>(*parent).cloneEmpty();
  }

  ws->initialize(numSpectra,
                 HistogramData::Histogram(std::forward<HistArgs>(histArgs)...));
  WorkspaceFactory::Instance().initializeFromParent(
      parent, ws, parent->y(0).size() != ws->y(0).size());

  return std::move(ws);
}
*/

}
}

#endif /*MANTID_KERNEL_WORKSPACEFACTORY_H_*/
