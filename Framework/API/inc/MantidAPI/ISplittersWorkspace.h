#ifndef MANTID_API_ISPLITTERSWORKSPACE_H_
#define MANTID_API_ISPLITTERSWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TimeSplitter.h"

namespace Mantid {
namespace API {

/** ISplittersWorkspace : Workspace to contain splitters for event filtering.
 * It inherits from ITableWorkspace

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
class MANTID_API_DLL ISplittersWorkspace {
public:
  /*
   * Constructor
   */
  ISplittersWorkspace() = default;
  ISplittersWorkspace &operator=(const ISplittersWorkspace &other) = delete;
  /*
   * Destructor
   */
  virtual ~ISplittersWorkspace() = default;

  /// Returns a clone of the workspace
  std::unique_ptr<ISplittersWorkspace> clone() const {
    return std::unique_ptr<ISplittersWorkspace>(doClone());
  }

  /*
   * Add a time splitter to table workspace
   */
  virtual void addSplitter(Kernel::SplittingInterval splitter) = 0;

  /*
   * Get the corresponding workspace index of a time
   * Input time must the total nanoseconds of the absolute time from 1990.00.00
   */
  virtual Kernel::SplittingInterval getSplitter(size_t index) = 0;

  /*
   * Get number of splitters
   */
  virtual size_t getNumberSplitters() const = 0;

  /*
   * Remove one entry of a splitter
   */
  virtual bool removeSplitter(size_t splitterindex) = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  ISplittersWorkspace(const ISplittersWorkspace &) = default;

private:
  virtual ISplittersWorkspace *doClone() const = 0;
};

/// Typedef for a shared pointer to \c TableWorkspace
using ISplittersWorkspace_sptr = boost::shared_ptr<ISplittersWorkspace>;
/// Typedef for a shared pointer to \c const \c TableWorkspace
using ISplittersWorkspace_const_sptr =
    boost::shared_ptr<const ISplittersWorkspace>;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ISPLITTERSWORKSPACE_H_ */
