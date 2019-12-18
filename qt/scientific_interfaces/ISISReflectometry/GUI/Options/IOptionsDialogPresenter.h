// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGPRESENTER_H

#include "Common/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogMainWindowSubscriber {
public:
  virtual void optionsChanged() const = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IOptionsDialogPresenter {
public:
  virtual ~IOptionsDialogPresenter() = default;
  virtual void notifyInitOptions() = 0;
  virtual void notifyOptionsChanged() = 0;
  virtual void notifySubscribeView() = 0;
  virtual bool getBoolOption(std::string &optionName) = 0;
  virtual int &getIntOption(std::string &optionName) = 0;
  virtual void showView() = 0;
  virtual void subscribe(OptionsDialogMainWindowSubscriber *notifyee) = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGPRESENTER_H */
