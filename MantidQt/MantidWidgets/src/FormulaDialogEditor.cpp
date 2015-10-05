#include "MantidQtMantidWidgets/FormulaDialogEditor.h"
#include "MantidQtMantidWidgets/UserFunctionDialog.h"

#include <QFileDialog>
#include <QSettings>

namespace MantidQt
{
namespace MantidWidgets
{

/**
 * Open a UserFunctionDialog. Update the property if a file was selected.
 */
void FormulaDialogEditor::runDialog()
{
    MantidQt::MantidWidgets::UserFunctionDialog *dlg = new MantidQt::MantidWidgets::UserFunctionDialog((QWidget*)parent(),getText());
    if (dlg->exec() == QDialog::Accepted)
    {
      setText(dlg->getFormula());
      updateProperty();
    }
}

}
}
