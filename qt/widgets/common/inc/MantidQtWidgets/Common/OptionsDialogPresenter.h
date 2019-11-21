// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H
#define MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H

#include "MantidQtWidgets/Common/OptionsDialog.h"
#include "MantidQtWidgets/Common/OptionsDialogModel.h"

#include "DllOption.h"

//------------------------------------------------

namespace MantidQt {
namespace MantidWidgets {

/**
Implements a presenter for the options dialog.
*/
class EXPORT_OPT_MANTIDQT_COMMON OptionsDialogPresenter
    : public OptionsDialogSubscriber {
public:
  OptionsDialogPresenter(IOptionsDialog *view);
  ~OptionsDialogPresenter() = default;
  
  void loadOptions() override;
  void saveOptions() override;
  void showView();

private:
  // Settings
  void loadSettings(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions);
  void saveSettings(const std::map<std::string, bool> &boolOptions,
                    const std::map<std::string, int> &intOptions);
  void initOptions();
  void applyDefaultOptions(std::map<std::string, bool> &boolOptions,
                           std::map<std::string, int> &intOptions);

private:
  // Handle to the view for this presenter
  IOptionsDialog *m_view;
  // Handle to the model for this presenter
  OptionsDialogModel m_model;
  // stores the user options for the presenter
  std::map<std::string, bool> m_boolOptions;
  std::map<std::string, int> m_intOptions;
  const std::string REFLECTOMETRY_SETTINGS_GROUP = "ISISReflectometryUI";
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // OPTIONSDIALOGPRESENTER_H
