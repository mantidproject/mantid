// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_OPTIONSDIALOG_H
#define MANTID_ISISREFLECTOMETRY_OPTIONSDIALOG_H

#include "IOptionsDialog.h"

#include "Common/DllConfig.h"
#include "ui_OptionsDialog.h"
#include <map>
#include <QDialog>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** OptionsDialog : Provides a dialog for setting options.
 */

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialog : public QDialog,
                                                 public IOptionsDialog {
  Q_OBJECT
public:
  OptionsDialog(QWidget *parent);
  ~OptionsDialog() override;
  void getOptions(std::map<std::string, bool> &boolOptions,
                  std::map<std::string, int> &intOptions) override;
  void setOptions(std::map<std::string, bool> &boolOptions,
                  std::map<std::string, int> &intOptions) override;
  void show() override;
  void subscribe(OptionsDialogSubscriber *notifyee) override;

public slots:
  void notifyLoadOptions();
  void notifySaveOptions();
  void closeEvent(QCloseEvent *event) override;

private:
  void initLayout();
  void initBindings();

private:
  Ui::OptionsDialog m_ui;
  // subscribe updates from view
  OptionsDialogSubscriber *m_notifyee;
  // maps option names to widget names
  std::map<QString, QString> m_bindings;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_OPTIONSDIALOG_H */
