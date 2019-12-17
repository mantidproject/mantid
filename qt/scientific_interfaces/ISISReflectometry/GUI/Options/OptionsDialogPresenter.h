// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H

#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "QtOptionsDialogView.h"
#include "OptionsDialogModel.h"

#include "Common/DllConfig.h"

//------------------------------------------------

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/**
Implements a presenter for the options dialog.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogPresenter
    : public OptionsDialogSubscriber {
public:
  OptionsDialogPresenter(IOptionsDialogView *view,
                         IMainWindowPresenter *mainPresenter);
  ~OptionsDialogPresenter() = default;

  void notifyInitOptions();
  void notifySubscribe();
  void acceptMainPresenter(IMainWindowPresenter *mainPresenter);
  bool getBoolOption(std::string &optionName);
  int& getIntOption(std::string &optionName);
  void showView();

  // OptionsDialogSubscriber overrides
  void loadOptions() override;
  void saveOptions() override;

private:
  void initOptions();
  void notifyApplyDefaultOptions(std::map<std::string, bool> &boolOptions,
                                 std::map<std::string, int> &intOptions);

private:
  IOptionsDialogView *m_view;
  OptionsDialogModel m_model;
  IMainWindowPresenter *m_mainPresenter;
  // stores the user options for the presenter
  std::map<std::string, bool> m_boolOptions;
  std::map<std::string, int> m_intOptions;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H
