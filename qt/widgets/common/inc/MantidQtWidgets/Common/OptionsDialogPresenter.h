// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H
#define MANTIDQT_MANTIDWIDGETS_OPTIONSDIALOGPRESENTER_H

#include "MantidQtWidgets/Common/IOptionsDialog.h"
#include "MantidQtWidgets/Common/OptionsDialog.h"

#include "DllOption.h"

//------------------------------------------------

namespace MantidQt {
namespace MantidWidgets {

/**
Implements a presenter for the options dialog.
*/
class EXPORT_OPT_MANTIDQT_COMMON OptionsDialogPresenter {
public:
  OptionsDialogPresenter(OptionsDialog *view);

private:
  // TODO

  /// Handle to the view for this presenter
  OptionsDialog *m_view;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // OPTIONSDIALOGPRESENTER_H
