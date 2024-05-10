// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//--------------------------------------------------
// Includes
//--------------------------------------------------
#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidDialog.h"

#include <QDialog>
#include <QTreeWidgetItem>
#include <map>

namespace Ui {
class SelectFunctionDialog;
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Select a function type out of a list of available ones.
 */
class EXPORT_OPT_MANTIDQT_COMMON SelectFunctionDialog : public API::MantidDialog {
  Q_OBJECT

public:
  /// Default constructor
  SelectFunctionDialog(QWidget *parent = nullptr);
  /// Constructor overload with categories to restrict to
  SelectFunctionDialog(QWidget *parent, const std::vector<std::string> &restrictions);
  ~SelectFunctionDialog() override;
  /// Return selected function
  QString getFunction() const;
  /// Clear the text in the search box
  void clearSearchBoxText() const;

protected:
  /// Ui elements form
  Ui::SelectFunctionDialog *m_form;

private:
  /// Complete QComboBox for searching functions with available functions
  void addSearchBoxFunctionNames(const std::vector<std::string> &registeredFunctions);
  /// Construct QTreeWidget with categories and functions
  void constructFunctionTree(const std::map<std::string, std::vector<std::string>> &categoryFunctionsMap,
                             const std::vector<std::string> &restrictions);
  /// Sets the minimum height of the function tree so that all catagories are visible
  void setMinimumHeightOfFunctionTree();

private slots:
  void searchBoxChanged(const QString &text);
  void functionDoubleClicked(QTreeWidgetItem *item);
  void acceptFunction();
  void rejectFunction();
  void helpClicked() const;
};

} // namespace MantidWidgets
} // namespace MantidQt
