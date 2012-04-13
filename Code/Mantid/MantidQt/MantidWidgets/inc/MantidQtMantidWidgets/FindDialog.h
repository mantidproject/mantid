#ifndef FINDDIALOG_H_
#define FINDDIALOG_H_

//--------------------------------------------------
// Includes
//--------------------------------------------------
#include "MantidQtMantidWidgets/FindReplaceDialog.h"

/**
 * Specialisation of FindReplaceDialog that only
 * does finding
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS FindDialog : public FindReplaceDialog
{
  Q_OBJECT

public:
  FindDialog(ScriptEditor *editor, Qt::WindowFlags flags = 0);

};

#endif // FINDDIALOG_H_

