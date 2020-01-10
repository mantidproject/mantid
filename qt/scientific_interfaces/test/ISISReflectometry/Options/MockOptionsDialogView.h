// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H
#define MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H

#include "GUI/Options/IOptionsDialogView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MockOptionsDialogView : public IOptionsDialogView {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD2(getOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD2(setOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD1(subscribe, void(OptionsDialogViewSubscriber *notifyee));
  MOCK_METHOD0(show, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H */