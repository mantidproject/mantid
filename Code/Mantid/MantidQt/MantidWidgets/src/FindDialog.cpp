//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtMantidWidgets/FindDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QRegExp>
#include <QGridLayout>
#include <QVBoxLayout>


FindDialog::FindDialog(ScriptEditor *editor, Qt::WindowFlags flags)
  : FindReplaceDialog(editor, flags)
{
  setWindowTitle(tr("MantidPlot") + " - " + tr("Find"));
  initLayout();
}
