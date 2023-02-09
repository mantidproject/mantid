// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtWidgets/Common/FindDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FindDialog::FindDialog(ScriptEditor *editor) : FindReplaceDialog(editor) {
  setWindowTitle(QCoreApplication::applicationName() + " - Find");
  initLayout();
}
