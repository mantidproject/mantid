// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H
#define MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H

#include "MantidKernel/WarningSuppressions.h"
#include "GUI/Options/IOptionsDialogView.h"
#include <QVariant>
#include <gmock/gmock.h>
#include <map>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class MockOptionsDialogView : public IOptionsDialogView {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD2(getOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD2(setOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD1(
      subscribe,
      void(OptionsDialogSubscriber *notifyee));
  MOCK_METHOD0(show, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_ISISREFLECTOMETRY_MOCKOPTIONSDIALOGVIEW_H */