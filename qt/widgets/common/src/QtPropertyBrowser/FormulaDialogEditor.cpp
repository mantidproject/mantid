// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/FormulaDialogEditor.h"
#include "MantidQtWidgets/Common/UserFunctionDialog.h"

#include <QSettings>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Open a UserFunctionDialog. Update the property if a file was selected.
 */
void FormulaDialogEditor::runDialog() {
  MantidQt::MantidWidgets::UserFunctionDialog dlg((QWidget *)parent(),
                                                  getText());
  if (dlg.exec() == QDialog::Accepted) {
    setText(dlg.getFormula());
    updateProperty();
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
