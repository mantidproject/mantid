// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FilenameDialogEditor.h"

#include <QFileDialog>
#include <QSettings>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Open a file dialog to choose a file. Update the property if a file was
 * selected.
 */
void FilenameDialogEditor::runDialog() {
  QSettings settings;
  QString dir = settings.value("Mantid/FitBrowser/ResolutionDir").toString();
  QString StringDialog = QFileDialog::getOpenFileName(this, tr("Open File"), dir);
  if (!StringDialog.isEmpty()) {
    setText(StringDialog);
    updateProperty();
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
