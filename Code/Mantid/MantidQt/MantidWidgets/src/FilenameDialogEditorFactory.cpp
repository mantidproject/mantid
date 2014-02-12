#include "MantidQtMantidWidgets/FilenameDialogEditorFactory.h"

#include <QFileDialog>
#include <QSettings>

namespace MantidQt
{
namespace MantidWidgets
{

void FilenameDialogEditor::runDialog()
{
  QSettings settings;
  QString dir = settings.value("Mantid/FitBrowser/ResolutionDir").toString();
  QString StringDialog = QFileDialog::getOpenFileName(this, tr("Open File"),dir);
  if (!StringDialog.isEmpty())
  {
    setText(StringDialog);
    updateProperty();
  }
}

}
}
