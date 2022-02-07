// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** HardThresholdBackground : Implementation of BackgroundStrategy using a fixed
  background signal value as the threshold.
*/
class MANTID_CRYSTAL_DLL HardThresholdBackground : public BackgroundStrategy {
public:
  /// Contructor
  HardThresholdBackground(const double thresholdSignal, const Mantid::API::MDNormalization normalization);

  /// Overriden isBackground
  bool isBackground(Mantid::API::IMDIterator *iterator) const override;

  /// Overriden configureIterator.
  void configureIterator(Mantid::API::IMDIterator *const iterator) const override;

  /// Virtual constructor
  HardThresholdBackground *clone() const override;

private:
  /// Cutoff
  double m_thresholdSignal;
  /// Normalization
  Mantid::API::MDNormalization m_normalization;
};

} // namespace Crystal
} // namespace Mantid
