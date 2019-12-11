// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGVIEW_H
#define MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGVIEW_H

#include "Common/DllConfig.h"
#include <QVariant>
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogSubscriber {
public:
  virtual void loadOptions() = 0;
  virtual void saveOptions() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IOptionsDialogView {
public:
  virtual ~IOptionsDialogView() = default;
  virtual void getOptions(std::map<std::string, bool> &boolOptions,
                          std::map<std::string, int> &intOptions) = 0;
  virtual void setOptions(std::map<std::string, bool> &boolOptions,
                          std::map<std::string, int> &intOptions) = 0;
  virtual void show() = 0;
  virtual void subscribe(OptionsDialogSubscriber *notifyee) = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_IOPTIONSDIALOGVIEW_H */
