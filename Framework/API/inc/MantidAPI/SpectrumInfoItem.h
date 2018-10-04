// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/

class MANTID_API_DLL SpectrumInfoItem {

public:
  // Methods that can be called via the iterator
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

  const Mantid::SpectrumDefinition &spectrumDefinition() const {
    return m_spectrumInfo->spectrumDefinition(m_index);
  }

  Mantid::Kernel::V3D position() const {
    return m_spectrumInfo->position(m_index);
  }

private:
  // Allow SpectrumInfoIterator access
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
