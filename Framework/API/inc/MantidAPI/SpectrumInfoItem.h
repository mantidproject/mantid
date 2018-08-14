#ifndef MANTID_API_SPECTRUMINFOITEM_H_
#define MANTID_API_SPECTRUMINFOITEM_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/V3D.h"
#include "MantidTypes/SpectrumDefinition.h"

using Mantid::API::SpectrumInfo;
using Mantid::Kernel::V3D;
using Mantid::SpectrumDefinition;

namespace Mantid {
namespace API {

/** SpectrumInfoItem

SpectrumInfoItem is only created by SpectrumInfoIterator and allows users of
the SpectrumInfoIterator object access to data from SpectrumInfo. The available
methods include:
- isMonitor()
- isMaksed()
- twoTheta()
- signedTwoTheta()
- l2()
- hasUniqueDetector()
- spectrumDefinition()
- position()

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

class MANTID_API_DLL SpectrumInfoItem {

public:
  // Provide copy and move constructors
  SpectrumInfoItem(const SpectrumInfoItem &other) = default;
  SpectrumInfoItem &operator=(const SpectrumInfoItem &rhs) = default;
  SpectrumInfoItem(SpectrumInfoItem &&other) = default;
  SpectrumInfoItem &operator=(SpectrumInfoItem &&rhs) = default;

  bool isMonitor() const { return m_spectrumInfo->isMonitor(m_index); }

  bool isMasked() const { return m_spectrumInfo->isMasked(m_index); }

  double twoTheta() const { return m_spectrumInfo->twoTheta(m_index); }

  double signedTwoTheta() const {
    return m_spectrumInfo->signedTwoTheta(m_index);
  }

  double l2() const { return m_spectrumInfo->l2(m_index); }

  bool hasUniqueDetector() const {
    return m_spectrumInfo->hasUniqueDetector(m_index);
  }

  Mantid::SpectrumDefinition spectrumDefinition() const {
    return m_spectrumInfo->spectrumDefinition(m_index);
  }

  Mantid::Kernel::V3D position() const {
    return m_spectrumInfo->position(m_index);
  }

  void advance(int64_t delta) {
    m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
                                   static_cast<uint64_t>(m_index) + delta)
                        : std::min(m_spectrumInfo->size(),
                                   m_index + static_cast<size_t>(delta));
  }

  // This could cause a segmentation fault if a user goes past the end of the
  // iterator and tries to index into the n+1 th element (which would not
  // exist). Adding range checks to all the above methods may slow down
  // performance though.
  void incrementIndex() {
    if (m_index < m_spectrumInfo->size()) {
      ++m_index;
    }
  }

  void decrementIndex() {
    if (m_index > 0) {
      --m_index;
    }
  }

  size_t getIndex() const { return m_index; }

  void setIndex(const size_t index) { m_index = index; }

private:
  friend class SpectrumInfoIterator;

  // Private constructor, can only be created by SpectrumInfoIterator
  SpectrumInfoItem(const SpectrumInfo &spectrumInfo, const size_t index)
      : m_spectrumInfo(&spectrumInfo), m_index(index) {}

  // Non-owning pointer. A reference makes the class unable to define an
  // assignment operator that we need.
  const SpectrumInfo *m_spectrumInfo;
  size_t m_index;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRUMINFOITEM_H_ */
