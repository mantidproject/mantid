// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/** IALCBaselineModellingPresenterSubscriber : Subscriber's interface for getting updates from
 * ALCBaselineModellingPresenter
 */
class MANTIDQT_MUONINTERFACE_DLL IALCBaselineModellingPresenterSubscriber {

public:
  virtual ~IALCBaselineModellingPresenterSubscriber() = default;

  /// Notify the subscriber that the corrected baseline has changed.
  virtual void correctedDataChanged() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
