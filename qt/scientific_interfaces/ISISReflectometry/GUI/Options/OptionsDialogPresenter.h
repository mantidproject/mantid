// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H

#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "IOptionsDialogModel.h"
#include "IOptionsDialogPresenter.h"
#include "QtOptionsDialogView.h"

#include "Common/DllConfig.h"

//------------------------------------------------

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/**
Implements a presenter for the options dialog.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogPresenter
    : public IOptionsDialogPresenter,
      public OptionsDialogViewSubscriber {
public:
  explicit OptionsDialogPresenter(IOptionsDialogView *view,
                                  std::unique_ptr<IOptionsDialogModel> model);
  // IOptionsDialogPresenter overrides
  void notifyInitOptions() override;
  void notifyOptionsChanged() override;
  void notifySubscribeView() override;
  bool getBoolOption(std::string &optionName) override;
  int &getIntOption(std::string &optionName) override;
  void showView() override;
  void subscribe(OptionsDialogMainWindowSubscriber *notifyee) override;

  // OptionsDialogViewSubscriber overrides
  void loadOptions() override;
  void saveOptions() override;

private:
  void initOptions();

protected:
  // stores the user options for the presenter
  std::map<std::string, bool> m_boolOptions;
  std::map<std::string, int> m_intOptions;
  void notifyApplyDefaultOptions(std::map<std::string, bool> &boolOptions,
                                 std::map<std::string, int> &intOptions);

private:
  IOptionsDialogView *m_view;
  std::unique_ptr<IOptionsDialogModel> m_model;
  // subscribe updates from presenter
  OptionsDialogMainWindowSubscriber *m_mainWindowNotifyee;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_ISISREFLECTOMETRY_OPTIONSDIALOGPRESENTER_H
