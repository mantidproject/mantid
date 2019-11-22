// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_MOCKOPTIONSDIALOG_H
#define MANTID_MANTIDWIDGETS_MOCKOPTIONSDIALOG_H

#include "IOptionsDialog.h"
#include <gmock/gmock.h>
#include <map>
#include <QVariant>

class MockOptionsDialog : public MantidQt::MantidWidgets::IOptionsDialog {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD2(getOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD2(setOptions, void(std::map<std::string, bool> &boolOptions,
                                std::map<std::string, int> &intOptions));
  MOCK_METHOD1(subscribe, void(MantidQt::MantidWidgets::OptionsDialogSubscriber *notifyee));
  MOCK_METHOD0(show, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_MANTIDWIDGETS_MOCKOPTIONSDIALOG_H */