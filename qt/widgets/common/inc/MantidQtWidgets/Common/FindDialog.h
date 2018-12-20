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
  FindDialog(ScriptEditor *editor, Qt::WindowFlags flags = nullptr);
};

#endif // FINDDIALOG_H_
