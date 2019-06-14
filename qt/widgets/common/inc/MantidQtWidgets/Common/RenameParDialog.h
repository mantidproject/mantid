// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_
#define MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_

#include "DllOption.h"
#include "ui_RenameParDialog.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
/**
 * A dialog for renaming parameters for a user function
 */
class EXPORT_OPT_MANTIDQT_COMMON RenameParDialog : public QDialog {
  Q_OBJECT

public:
  /// there has to be a default constructor but you can call it with a pointer
  /// to the thing that will take ownership of it
  RenameParDialog(const std::vector<std::string> &old_params,
                  const std::vector<std::string> &new_params,
                  QWidget *parent = nullptr);
  std::vector<std::string> setOutput() const;
protected slots:
  void uniqueIndexedNames(bool /*ok*/);
  void doNotRename(bool /*ok*/);

protected:
  bool isUnique(const QString &name) const;
  QString makeUniqueIndexedName(const QString &name);
  /// User interface elements
  Ui::RenameParDialog m_uiForm;
  const std::vector<std::string> m_old_params;
  const std::vector<std::string> m_new_params;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_
