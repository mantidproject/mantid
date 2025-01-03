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

/** IALCDataLoadingPresenterSubscriber : Subscriber's interface for getting updates from
 * ALCDataLoadingPresenter
 */
class MANTIDQT_MUONINTERFACE_DLL IALCDataLoadingPresenterSubscriber {

public:
  virtual ~IALCDataLoadingPresenterSubscriber() = default;

  /// Notify the subscriber that the loaded data has changed.
  virtual void loadedDataChanged() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
