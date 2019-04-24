// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_SELECTFUNCTIONDIALOG_H_
#define MANTIDWIDGETS_SELECTFUNCTIONDIALOG_H_

//--------------------------------------------------
// Includes
//--------------------------------------------------
#include "DllOption.h"

#include <QDialog>
#include <map>

namespace Ui {
class SelectFunctionDialog;
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Select a function type out of a list of available ones.
 */
class EXPORT_OPT_MANTIDQT_COMMON SelectFunctionDialog : public QDialog {
  Q_OBJECT

public:
  /// Default constructor
  SelectFunctionDialog(QWidget *parent = nullptr);
  /// Constructor overload with categories to restrict to
  SelectFunctionDialog(QWidget *parent,
                       const std::vector<std::string> &restrictions);
  ~SelectFunctionDialog() override;
  /// Return selected function
  QString getFunction() const;

protected:
  /// Ui elements form
  Ui::SelectFunctionDialog *m_form;

private:
  /// Construct QTreeWidget with categories and functions
  void
  constructFunctionTree(const std::map<std::string, std::vector<std::string>>
                            &categoryFunctionsMap,
                        const std::vector<std::string> &restrictions);
};

} // MantidQt
} // MantidWidgets

#endif // MANTIDWIDGETS_SELECTFUNCTIONDIALOG_H_
