// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogViewSubscriber {
public:
  virtual void notifyLoadOptions() = 0;
  virtual void notifySaveOptions() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IOptionsDialogView {
public:
  virtual ~IOptionsDialogView() = default;
  virtual void getOptions(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions) = 0;
  virtual void setOptions(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions) = 0;
  virtual void show() = 0;
  virtual void subscribe(OptionsDialogViewSubscriber *notifyee) = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
