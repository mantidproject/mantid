#ifndef MANTID_DATAOBJECTS_WORKSPACECREATION_H_
#define MANTID_DATAOBJECTS_WORKSPACECREATION_H_

#include "MantidHistogramData/Histogram.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/DllConfig.h"

#include <memory>

namespace Mantid {
namespace DataObjects {
class EventWorkspace;
class HistoWorkspace;
class Workspace2D;

/** WorkspaceCreation : TODO: DESCRIPTION

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
MANTID_DATAOBJECTS_DLL std::unique_ptr<Workspace2D> createWorkspace2D();
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
    ws = detail::createWorkspace2D();
  } else {
    // This may throw std::bad_cast.
    ws = dynamic_cast<const T &>(*parent).cloneEmpty();
  }

  ws->initialize(numSpectra,
                 HistogramData::Histogram(std::forward<HistArgs>(histArgs)...));
  API::WorkspaceFactory::Instance().initializeFromParent(
      parent, ws, parent->y(0).size() != ws->y(0).size());

  return std::move(ws);
}

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_WORKSPACECREATION_H_ */
