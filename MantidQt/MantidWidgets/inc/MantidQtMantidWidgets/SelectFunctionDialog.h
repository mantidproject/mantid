#ifndef FINDDIALOG_H_
#define FINDDIALOG_H_

//--------------------------------------------------
// Includes
//--------------------------------------------------
#include "WidgetDllOption.h"

#include <QDialog>

namespace Ui
{
  class SelectFunctionDialog;
}

/**
 * Select a function type out of a list of available ones.
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS SelectFunctionDialog : public QDialog
{
  Q_OBJECT

public:
  SelectFunctionDialog(QWidget *parent = NULL);
  ~SelectFunctionDialog();
  /// Return selected function
  QString getFunction() const;
protected:
  /// Ui elements form
  Ui::SelectFunctionDialog *m_form;
};

#endif // FINDDIALOG_H_

