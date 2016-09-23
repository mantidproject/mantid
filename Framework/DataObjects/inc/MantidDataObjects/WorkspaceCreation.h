#ifndef MANTID_DATAOBJECTS_WORKSPACECREATION_H_
#define MANTID_DATAOBJECTS_WORKSPACECREATION_H_

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/DllConfig.h"

#include <memory>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace API {
class MatrixWorkspace;
class HistoWorkspace;
}
namespace DataObjects {
class EventWorkspace;
class Workspace2D;

/** Factory methods for creating MatrixWorkspaces. A template parameter T
  specifies the type of (or a base type of) the created workspace:
  - The type of the output workspace is identical to T for the variants without
    parent.
  - The type of the output workspace is the dynamic type of the parent if T is a
    base of the parents dynamic type.
  - If T is not a base of the parents dynamic type, a conversion is attempted.
    Currently this is the case only for EventWorkspace:
    - If the dynamic type of the parent is EventWorkspace but T is not, either a
      Workspace2D or T is created, whichever is more derived. For example, a
      typical use-case is to drop events and create a Workspace2D from an
      EventWorkspace.  This can be achieved as follows:
      ~~~{.cpp}
      auto ws = create<HistoWorkspace>(parent);
      ~~~
      This is equivalent to the old way of using
      WorkspaceFactory::create(parent). In this case, Workspace2D is more
      derived than HistoWorkspace, so a Workspace2D is created.

  Other arguments can include:
  - The desired number of spectra (NumSpectra) to be created in the output
    workspace.
  - A reference to an IndexInfo object, defining the number of spectra and
    spectrum number and detector IDs.
  - A reference to a Histogram object, defining the size of the histograms as
    well as whether the workspace stores point data or histogram data. This is a
    replacement for an independent specification of the X and Y lengths. This is
    also used to initialize the workspace with X data and (optionally) Y and E
    data.

  Available variants are:

  ~~~{.cpp}
  create<T>(NumSpectra, Histogram)
  create<T>(IndexInfo,  Histogram)
  create<T>(ParentWS)
  create<T>(ParentWS, Histogram)
  create<T>(ParentWS, NumSpectra)
  create<T>(ParentWS, IndexInfo)
  create<T>(ParentWS, NumSpectra, Histogram)
  create<T>(ParentWS, IndexInfo, Histogram)
  ~~~

  - If neither NumSpectra nor IndexInfo is given, or if the new size is
    identical to the size of the parent, the created workspace has the same
    number of spectra as the parent workspace and spectrum number as well as
    detector ID information is copied from the parent.
  - If Histogram is not given, the created workspace has X identical to the
    parent workspace and Y and E are initialized to 0.
  - If a Histogram with 'NULL' Y and E is given, Y and E are initialized to 0.

  In all cases a (smart) pointer to T is returned.

  @author Simon Heybrock
  @date 2016

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace detail {
MANTID_DATAOBJECTS_DLL HistogramData::Histogram
stripData(HistogramData::Histogram histogram);

template <class T> std::unique_ptr<T> createHelper() {
  return Kernel::make_unique<T>();
}

template <class T> std::unique_ptr<T> createConcreteHelper() {
  return Kernel::make_unique<T>();
}

template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::HistoWorkspace> createHelper();

// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<EventWorkspace> createHelper();
// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::MatrixWorkspace> createHelper();

// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::MatrixWorkspace> createConcreteHelper();
// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::HistoWorkspace> createConcreteHelper();
}

template <class T, class P, class IndexArg,
          class = typename std::enable_if<
              std::is_base_of<API::MatrixWorkspace, P>::value>::type>
boost::shared_ptr<T> create(const P &parent, const IndexArg &indexArg,
                            const HistogramData::Histogram &histogram) {
  // Figure out (dynamic) target type:
  // - Type is same as parent if T is base of parent
  // - If T is not base of parent, conversion may occur. Currently only
  //   supported for EventWorkspace
  boost::shared_ptr<T> ws;
  if (std::is_base_of<API::HistoWorkspace, T>::value &&
      parent.id() == "EventWorkspace") {
    // Drop events, create Workspace2D or T whichever is more derived.
    ws = detail::createHelper<T>();
  } else {
    try {
      // If parent is more derived than T: create type(parent)
      ws = dynamic_cast<const T &>(parent).cloneEmpty();
    } catch (std::bad_cast &) {
      // If T is more derived than parent: create T
      ws = detail::createConcreteHelper<T>();
    }
  }

  ws->initialize(indexArg, histogram);
  API::WorkspaceFactory::Instance().initializeFromParent(
      parent, ws, parent.y(0).size() != ws->y(0).size());
  // For EventWorkspace, `ws->y(0)` put entry 0 in the MRU. However, clients
  // would typically expect an empty MRU and fail to clear it. This dummy call
  // removes the entry from the MRU.
  static_cast<void>(ws->mutableX(0));

  return std::move(ws);
}

template <class T, class IndexArg,
          typename std::enable_if<
              !std::is_base_of<API::MatrixWorkspace, IndexArg>::value>::type * =
              nullptr>
boost::shared_ptr<T> create(const IndexArg &indexArg,
                            const HistogramData::Histogram &histogram) {
  auto ws = Kernel::make_unique<T>();
  ws->initialize(indexArg, histogram);
  return std::move(ws);
}

template <class T, class P,
          typename std::enable_if<std::is_base_of<API::MatrixWorkspace,
                                                  P>::value>::type * = nullptr>
boost::shared_ptr<T> create(const P &parent) {
  return create<T>(parent, detail::stripData(parent.histogram(0)));
}

template <class T, class P, class IndexArg,
          typename std::enable_if<std::is_base_of<API::MatrixWorkspace,
                                                  P>::value>::type * = nullptr>
boost::shared_ptr<T> create(const P &parent, const IndexArg &indexArg) {
  return create<T>(parent, indexArg, detail::stripData(parent.histogram(0)));
}

template <class T, class P,
          typename std::enable_if<std::is_base_of<API::MatrixWorkspace,
                                                  P>::value>::type * = nullptr>
boost::shared_ptr<T> create(const P &parent,
                            const HistogramData::Histogram &histogram) {
  return create<T>(parent, parent.getNumberHistograms(), histogram);
}

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_WORKSPACECREATION_H_ */
