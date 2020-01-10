// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGMODEL_H
#define MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGMODEL_H

#include "GUI/Options/IOptionsDialogModel.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MockOptionsDialogModel : public IOptionsDialogModel {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD2(applyDefaultOptionsProxy, void(std::map<std::string, bool> &,
                                              std::map<std::string, int> &));
  void applyDefaultOptions(std::map<std::string, bool> &boolOptions,
                           std::map<std::string, int> &intOptions) {
    boolOptions["WarnProcessAll"] = false;
    boolOptions["WarnDiscardChanges"] = false;
    boolOptions["WarnProcessPartialGroup"] = false;
    boolOptions["Round"] = true;
    intOptions["RoundPrecision"] = 5;
    applyDefaultOptionsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(loadSettingsProxy, void(std::map<std::string, bool> &,
                                       std::map<std::string, int> &));
  void loadSettings(std::map<std::string, bool> &boolOptions,
                    std::map<std::string, int> &intOptions) {
    boolOptions["WarnProcessAll"] = false;
    boolOptions["WarnDiscardChanges"] = true;
    boolOptions["WarnProcessPartialGroup"] = false;
    boolOptions["Round"] = true;
    intOptions["RoundPrecision"] = 2;
    loadSettingsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(saveSettings, void(const std::map<std::string, bool> &,
                                  const std::map<std::string, int> &));
};

class MockOptionsDialogModelUnsuccessfulLoad : public IOptionsDialogModel {
public:
  MOCK_METHOD2(applyDefaultOptionsProxy, void(std::map<std::string, bool> &,
                                              std::map<std::string, int> &));
  void applyDefaultOptions(std::map<std::string, bool> &boolOptions,
                           std::map<std::string, int> &intOptions) {
    applyDefaultOptionsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(loadSettingsProxy, void(std::map<std::string, bool> &,
                                       std::map<std::string, int> &));
  void loadSettings(std::map<std::string, bool> &boolOptions,
                    std::map<std::string, int> &intOptions) {
    loadSettingsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(saveSettings, void(const std::map<std::string, bool> &,
                                  const std::map<std::string, int> &));
};

class MockOptionsDialogModelUnsuccessfulDefaults : public IOptionsDialogModel {
public:
  MOCK_METHOD2(applyDefaultOptionsProxy, void(std::map<std::string, bool> &,
                                              std::map<std::string, int> &));
  void applyDefaultOptions(std::map<std::string, bool> &boolOptions,
                           std::map<std::string, int> &intOptions) {
    boolOptions["WarnProcessAll"] = false;
    boolOptions["WarnDiscardChanges"] = false;
    boolOptions["WarnProcessPartialGroup"] = false;
    boolOptions["Round"] = true;
    intOptions["RoundPrecision"] = 5;
    applyDefaultOptionsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(loadSettingsProxy, void(std::map<std::string, bool> &,
                                       std::map<std::string, int> &));
  void loadSettings(std::map<std::string, bool> &boolOptions,
                    std::map<std::string, int> &intOptions) {
    loadSettingsProxy(boolOptions, intOptions);
  }
  MOCK_METHOD2(saveSettings, void(const std::map<std::string, bool> &,
                                  const std::map<std::string, int> &));
};
  GNU_DIAG_ON_SUGGEST_OVERRIDE
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGMODEL_H */