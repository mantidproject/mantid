#ifndef MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_
#define MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_

#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/TimeSplitter.h"

#ifdef _MSC_VER
#pragma warning(                                                               \
    disable : 4250) // Disable warning regarding inheritance via dominance
#endif

namespace Mantid {
namespace DataObjects {

/** SplittersWorkspace : A TableWorkspace to contain TimeSplitters.
  It will be used as an input for FilterEvents, which is the ultimate method for
  event filtering.
  There can be various algorithms to generate an object like this.

  A SplittersWorkspace contains 3 columns as int64, int64 and int32 to denote
  (1) splitter start time (2) splitter end time and (3) group workspace index

  @date 2012-04-03

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SplittersWorkspace : public DataObjects::TableWorkspace,
                                     public API::ISplittersWorkspace {
public:
  SplittersWorkspace();

  /// Returns a clone of the workspace
  std::unique_ptr<SplittersWorkspace> clone() const {
    return std::unique_ptr<SplittersWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<SplittersWorkspace> cloneEmpty() const {
    return std::unique_ptr<SplittersWorkspace>(doCloneEmpty());
  }

  SplittersWorkspace &operator=(const SplittersWorkspace &other) = delete;
  void addSplitter(Kernel::SplittingInterval splitter) override;

  Kernel::SplittingInterval getSplitter(size_t index) override;

  size_t getNumberSplitters() const override;

  bool removeSplitter(size_t) override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  SplittersWorkspace(const SplittersWorkspace &) = default;

private:
  SplittersWorkspace *doClone() const override {
    return new SplittersWorkspace(*this);
  }
  SplittersWorkspace *doCloneEmpty() const override {
    return new SplittersWorkspace();
  }
};

using SplittersWorkspace_sptr = boost::shared_ptr<SplittersWorkspace>;
using SplittersWorkspace_const_sptr =
    boost::shared_ptr<const SplittersWorkspace>;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_ */
