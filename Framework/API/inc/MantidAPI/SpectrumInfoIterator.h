#ifndef MANTID_API_SPECTRUMINFOITERATOR_H_
#define MANTID_API_SPECTRUMINFOITERATOR_H_

#include "MantidAPI/SpectrumInfoItem.h"

#include <boost/iterator/iterator_facade.hpp>

using Mantid::API::SpectrumInfoItem;

namespace Mantid {
namespace API {

/** SpectrumInfoIterator

SpectrumInfoIterator allows users of the SpectrumInfo object access to data
via an iterator. The iterator works as a slice view in that the index is
incremented and all items accessible at that index are made available via the
iterator.

@author Bhuvan Bezawada, STFC
@date 2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_API_DLL SpectrumInfoIterator
    : public boost::iterator_facade<SpectrumInfoIterator,
                                    const SpectrumInfoItem &,
                                    boost::random_access_traversal_tag> {

public:
  SpectrumInfoIterator(const SpectrumInfo &spectrumInfo, const size_t index)
      : m_item(spectrumInfo, index) {}

private:
  // Allow boost iterator access
  friend class boost::iterator_core_access;

  // Iterator methods
  void advance(int64_t delta) {
    m_item.m_index =
        delta < 0 ? std::max(static_cast<uint64_t>(0),
                             static_cast<uint64_t>(m_item.m_index) + delta)
                  : std::min(m_item.m_spectrumInfo->size(),
                             m_item.m_index + static_cast<size_t>(delta));
  }

  // This could cause a segmentation fault if a user goes past the end of the
  // iterator and tries to index into the n+1 th element (which would not
  // exist). Adding range checks to all the above methods may slow down
  // performance though.
  void increment() {
    if (m_item.m_index < m_item.m_spectrumInfo->size()) {
      ++m_item.m_index;
    }
  }

  void decrement() {
    if (m_item.m_index > 0) {
      --m_item.m_index;
    }
  }

  size_t getIndex() const { return m_item.m_index; }

  void setIndex(const size_t index) { m_item.m_index = index; }

  uint64_t distance_to(const SpectrumInfoIterator &other) const {
    return static_cast<uint64_t>(other.getIndex()) -
           static_cast<uint64_t>(getIndex());
  }

  bool equal(const SpectrumInfoIterator &other) const {
    return getIndex() == other.getIndex();
  }

  const SpectrumInfoItem &dereference() const { return m_item; }

  SpectrumInfoItem m_item;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMINFOITERATOR_H_ */
