// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H
#define MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H

#include "MantidQtWidgets/Common/OptionsDialog.h"

#include "DllOption.h"

//------------------------------------------------

namespace MantidQt {
namespace MantidWidgets {

/**
Implements a presenter for the options dialog.
*/
class OptionsDialogPresenter {
public:
  OptionsDialogPresenter(OptionsDialog *view);
  ~OptionsDialogPresenter() = default;

private slots:
  void loadOptions();
  void saveOptions();

private:
  // Settings
  void loadSettings(std::map<QString, QVariant> &options);
  void saveSettings(const std::map<QString, QVariant> &options);
  void initOptions();
  void applyDefaultOptions(std::map<QString, QVariant> &options);

private:
  // Handle to the view for this presenter
  OptionsDialog *m_view;

  // stores the user options for the presenter
  std::map<QString, QVariant> m_options;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // OPTIONSDIALOGPRESENTER_H
