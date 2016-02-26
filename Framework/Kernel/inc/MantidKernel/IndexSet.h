#ifndef MANTID_KERNEL_INDEXSET_H_
#define MANTID_KERNEL_INDEXSET_H_

#include <vector>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** IndexSet is a container that can be used to define and access a subset of
  elements in a larger container such as std::vector.

  In particular this is used when accessing the spectra in a workspace. Users
  frequently need to specify a certain range or list of spectra to use for an
  operation. This class provides a set of indices for this purpose.

  @author Simon Heybrock, ESS

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

class MANTID_KERNEL_DLL IndexSet {
public:
  IndexSet(size_t fullRange);
  IndexSet(int64_t min, int64_t max, size_t fullRange);
  IndexSet(const std::vector<size_t> indices, size_t fullRange);

  /// Returns the size of the set.
  size_t size() const { return m_size; }

  /// Returns the element at given index (range 0...size()-1).
  size_t operator[](size_t index) const {
    // This is accessed frequently in loops and thus inlined.
    if (m_isRange)
      return m_min + index;
    return m_indices[index];
  }

private:
  bool m_isRange = true;
  // Default here to avoid uninitialized warning for m_isRange = false.
  size_t m_min = 0;
  size_t m_size;
  std::vector<size_t> m_indices;
};

} // Namespace Kernel
} // Namespace Mantid

#endif /* MANTID_KERNEL_INDEXSET_H_ */
