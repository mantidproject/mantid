//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtWidgets/Common/FindDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRegExp>
#include <QVBoxLayout>

FindDialog::FindDialog(ScriptEditor *editor, Qt::WindowFlags flags)
    : FindReplaceDialog(editor, flags) {
  setWindowTitle(tr("MantidPlot") + " - " + tr("Find"));
  initLayout();
}
