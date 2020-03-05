// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FINDDIALOG_H_
#define FINDDIALOG_H_

//--------------------------------------------------
// Includes
//--------------------------------------------------
#include "MantidQtWidgets/Common/FindReplaceDialog.h"

/**
 * Specialisation of FindReplaceDialog that only
 * does finding
 */
class EXPORT_OPT_MANTIDQT_COMMON FindDialog : public FindReplaceDialog {
  Q_OBJECT

public:
  FindDialog(ScriptEditor *editor);
};

#endif // FINDDIALOG_H_
