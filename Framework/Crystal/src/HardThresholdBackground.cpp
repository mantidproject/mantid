// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidAPI/IMDIterator.h"

namespace Mantid {
namespace Crystal {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
HardThresholdBackground::HardThresholdBackground(
    const double thresholdSignal,
    const Mantid::API::MDNormalization normalization)
    : m_thresholdSignal(thresholdSignal), m_normalization(normalization) {}

HardThresholdBackground *HardThresholdBackground::clone() const {
  return new HardThresholdBackground(*this);
}

void HardThresholdBackground::configureIterator(
    Mantid::API::IMDIterator *const iterator) const {
  iterator->setNormalization(m_normalization);
}

bool HardThresholdBackground::isBackground(
    Mantid::API::IMDIterator *iterator) const {
  auto signal = iterator->getNormalizedSignal();
  return signal <= m_thresholdSignal || std::isnan(signal);
}

} // namespace Crystal
} // namespace Mantid
