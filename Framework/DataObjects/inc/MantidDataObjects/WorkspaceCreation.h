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

/** Factory methods for creating workspaces. A template parameter specifies the
  type of the created workspace (or a base type). If the parent passed as an
  argument is an EventWorkspace but the template parameter is not, the methods
  will attempt to create an adequate alternative. For example, a typical
  use-case is to drop events and create a Workspace2D from an EventWorkspace.
  This can be achieved as follows:

  ~~~{.cpp}
  auto ws = create<HistoWorkspace>(parent);
  ~~~

  This is equivalent to the old way of using WorkspaceFactory::create(parent).

  Available variants are:

  create(NumSpectra, Histogram)
  create(IndexInfo,  Histogram)
  create(ParentWS)
  create(ParentWS, Histogram)
  create(ParentWS, NumSpectra)
  create(ParentWS, IndexInfo)
  create(ParentWS, NumSpectra, Histogram)
  create(ParentWS, IndexInfo, Histogram)

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

template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::HistoWorkspace> createHelper();

// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<EventWorkspace> createHelper();
// Dummy specialization, should never be called, must exist for compilation.
template <>
MANTID_DATAOBJECTS_DLL std::unique_ptr<API::MatrixWorkspace> createHelper();
}

// IndexArg can be size_t or IndexInfo (forwarded to
// MatrixWorkspace::initialize).
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
    // This may throw std::bad_cast.
    ws = dynamic_cast<const T &>(parent).cloneEmpty();
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
