#include "MantidQtMantidWidgets/FormulaDialogEditor.h"
#include "MantidQtMantidWidgets/UserFunctionDialog.h"

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
}
}
