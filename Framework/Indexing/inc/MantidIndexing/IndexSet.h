#ifndef MANTID_INDEXING_INDEXSET_H_
#define MANTID_INDEXING_INDEXSET_H_

#include "MantidIndexing/DllConfig.h"

#include <set>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace Indexing {
namespace detail {

/** IndexSet : TODO: DESCRIPTION
  CRTP
  We want DetectorIndexSet and SpectrumIndexSet, with the same functionality,
  but the types should be incompatible. Thus, we use IndexSet<DetectorIndexSet>
  and IndexSet<SpectrumIndexSet>.

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
template <class T> class IndexSet {
public:
  IndexSet(size_t fullRange);
  IndexSet(int64_t min, int64_t max, size_t fullRange);
  IndexSet(const std::vector<size_t> &indices, size_t fullRange);

  /// Returns the size of the set.
  size_t size() const { return m_size; }

  /// Returns the element at given index (range 0...size()-1).
  size_t operator[](size_t index) const {
    // This is accessed frequently in loops and thus inlined.
    if (m_isRange)
      return m_min + index;
    return m_indices[index];
  }

protected:
  ~IndexSet() = default;

private:
  bool m_isRange = true;
  // Default here to avoid uninitialized warning for m_isRange = false.
  size_t m_min = 0;
  size_t m_size;
  std::vector<size_t> m_indices;
};

/// Constructor for a set covering the full range from 0 to fullRange-1.
template <class T>
IndexSet<T>::IndexSet(size_t fullRange)
    : m_size(fullRange) {}

/// Constructor for a set covering the range from min to max. Range is verified
/// at construction time.
template <class T>
IndexSet<T>::IndexSet(int64_t min, int64_t max, size_t fullRange) {
  if (min < 0 || min > max)
    throw std::logic_error("IndexSet: specified min or max values are invalid");
  if (max >= static_cast<int64_t>(fullRange))
    throw std::out_of_range("IndexSet: specified max value is out of range");

  // Bounds checked, cast should be fine in all cases.
  m_min = static_cast<size_t>(min);
  m_size = static_cast<size_t>(max - min + 1);
}

/// Constructor for a set containing all specified indices. Range is verified at
/// construction time and duplicates are removed.
template <class T>
IndexSet<T>::IndexSet(const std::vector<size_t> &indices, size_t fullRange)
    : m_isRange(false) {
  // We use a set to create unique and ordered indices.
  std::set<size_t> index_set(indices.cbegin(), indices.cend());
  if (!index_set.empty() && *(index_set.rbegin()) >= fullRange)
    throw std::out_of_range("IndexSet: specified index is out of range");
  m_indices = std::vector<size_t>(begin(index_set), end(index_set));
  m_size = m_indices.size();
}

} // namespace detail
} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXSET_H_ */
