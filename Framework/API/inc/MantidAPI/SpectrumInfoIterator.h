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
  friend class boost::iterator_core_access;

  void increment() { m_item.incrementIndex(); }

  bool equal(const SpectrumInfoIterator &other) const {
    return m_item.getIndex() == other.m_item.getIndex();
  }

  const SpectrumInfoItem &dereference() const { return m_item; }

  void decrement() { m_item.decrementIndex(); }

  void advance(int64_t delta) { m_item.advance(delta); }

  uint64_t distance_to(const SpectrumInfoIterator &other) const {
    return static_cast<uint64_t>(other.m_item.getIndex()) -
           static_cast<uint64_t>(m_item.getIndex());
  }

  SpectrumInfoItem m_item;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMINFOITERATOR_H_ */
