// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_OPTIONSDIALOG_H
#define MANTIDQTMANTIDWIDGETS_OPTIONSDIALOG_H

#include "IOptionsDialog.h"

#include "DllOption.h"
#include "ui_OptionsDialog.h"
#include <map>
#include <QDialog>
#include <QVariant>

namespace MantidQt {
namespace MantidWidgets {

/** OptionsDialog : Provides a dialog for setting options.
 */

class EXPORT_OPT_MANTIDQT_COMMON OptionsDialog : public QDialog,
                                                 public IOptionsDialog {
  Q_OBJECT
public:
  OptionsDialog(QWidget *parent);
  ~OptionsDialog() override;
  void getOptions(std::map<QString, QVariant> &options) override;
  void setOptions(std::map<QString, QVariant> &options) override;
  void show() override;

protected:
  void initLayout();
  void initBindings();

private:
  Ui::OptionsDialog m_ui;
  // maps option names to widget names
  std::map<QString, QString> m_bindings;

};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQTMANTIDWIDGETS_OPTIONSDIALOG_H */
